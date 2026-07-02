#include <mqtt/async_client.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <csignal>  // signal(), SIGINT
#include <ctime>    // time() cho srand()

#include "Device.h"
#include "TemperatureSensor.h"
#include "SmartLight.h"
#include "SecurityCamera.h"

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const int RECONNECT_DELAY_SEC = 5; // chờ bao lâu trước khi thử reconnect

// std::atomic<bool> để 2 thread (main + signal handler) đọc/ghi an toàn.
// volatile sig_atomic_t là kiểu C cũ, std::atomic<bool> là cách C++11+.
std::atomic<bool> g_running{true}; // false khi nhận Ctrl+C

// Signal handler: chỉ set flag, KHÔNG làm gì khác.
// Lý do: signal handler chạy trong ngữ cảnh đặc biệt, gọi hầu hết
// hàm C++ (cout, malloc...) từ đây là undefined behavior. Chỉ được
// ghi vào sig_atomic_t hoặc std::atomic.
void onSigInt(int) {
    g_running = false;
}

class CommandCallback : public virtual mqtt::callback {
private:
    Device&            device_;
    // Dùng atomic thay vì bool thường — vì connection_lost() chạy trên
    // thread riêng của Paho, main() đọc flag này từ thread chính.
    std::atomic<bool>& needsReconnect_;

public:
    CommandCallback(Device& device, std::atomic<bool>& needsReconnect)
        : device_(device), needsReconnect_(needsReconnect) {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string payload = msg->to_string();
        std::cout << "[COMMAND] Nhan duoc lenh: " << payload << "\n";
        if      (payload == "ON")  { device_.turnOn();  std::cout << "-> Da BAT.\n"; }
        else if (payload == "OFF") { device_.turnOff(); std::cout << "-> Da TAT.\n"; }
        else                       { std::cout << "-> Lenh khong hop le.\n"; }
    }

    void connection_lost(const std::string& cause) override {
        std::cout << "[WARN] Mat ket noi broker: "
                  << (cause.empty() ? "(khong ro nguyen nhan)" : cause) << "\n";
        // Chỉ set flag — KHÔNG gọi connect() ở đây.
        // Lý do: gọi connect() bên trong callback của Paho có thể gây
        // deadlock (Paho đang giữ lock nội bộ khi gọi callback này).
        needsReconnect_ = true;
    }
};

std::unique_ptr<Device> createDevice(const std::string& type,
                                      const std::string& id,
                                      const std::string& name) {
    if (type == "TemperatureSensor") return std::make_unique<TemperatureSensor>(id, name);
    if (type == "SmartLight")        return std::make_unique<SmartLight>(id, name);
    if (type == "SecurityCamera")    return std::make_unique<SecurityCamera>(id, name);
    return nullptr;
}

// Hàm helper: thực hiện connect + subscribe + publish ONLINE.
// Tách ra hàm riêng vì cần gọi ở cả lần đầu lẫn mỗi lần reconnect.
bool connectAndSetup(mqtt::async_client& client,
                     const mqtt::connect_options& connOpts,
                     const std::string& commandTopic,
                     const std::string& statusTopic) {
    try {
        std::cout << "[INFO] Dang ket noi toi broker...\n";
        client.connect(connOpts)->wait();
        std::cout << "[INFO] Ket noi thanh cong!\n";

        client.subscribe(commandTopic, 1)->wait();
        std::cout << "[INFO] Subscribe: " << commandTopic << "\n";

        auto onlineMsg = mqtt::make_message(statusTopic, "ONLINE");
        onlineMsg->set_qos(1);
        onlineMsg->set_retained(true);
        client.publish(onlineMsg)->wait();

        return true;
    } catch (const mqtt::exception& e) {
        std::cerr << "[ERROR] Ket noi that bai: " << e.what() << "\n";
        return false;
    }
}

int main(int argc, char* argv[]) {
    // Đăng ký signal handler cho Ctrl+C (SIGINT).
    // Từ giờ Ctrl+C sẽ set g_running=false thay vì kill process ngay.
    std::signal(SIGINT, onSigInt);

    srand(static_cast<unsigned>(time(nullptr)));

    if (argc < 4) {
        std::cerr << "Cach dung: " << argv[0] << " <Type> <ID> <Name>\n"
                  << "Vi du: " << argv[0]
                  << " TemperatureSensor TEMP_01 \"Cam bien phong khach\"\n";
        return 1;
    }

    auto devicePtr = createDevice(argv[1], argv[2], argv[3]);
    if (!devicePtr) {
        std::cerr << "[ERROR] Khong nhan dien duoc loai thiet bi: " << argv[1] << "\n";
        return 1;
    }

    Device& device = *devicePtr;
    device.turnOn();

    const std::string id          = device.getId();
    const std::string clientId    = "device_publisher_" + id;
    const std::string dataTopic   = "iot/device/" + id + "/data";
    const std::string statusTopic = "iot/device/" + id + "/status";
    const std::string commandTopic= "iot/device/" + id + "/command";

    std::atomic<bool> needsReconnect{false};

    mqtt::async_client client(SERVER_ADDRESS, clientId);

    CommandCallback cb(device, needsReconnect);
    client.set_callback(cb);

    mqtt::message willMsg(statusTopic, "OFFLINE", 1, true);
    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);
    connOpts.set_will(mqtt::will_options(willMsg));

    // Lần connect đầu tiên — nếu broker chưa chạy, thử lại mỗi 5 giây
    while (g_running && !connectAndSetup(client, connOpts, commandTopic, statusTopic)) {
        std::cout << "[INFO] Thu lai sau " << RECONNECT_DELAY_SEC << "s...\n";
        std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SEC));
    }

    // Vòng lặp chính: publish data + xử lý reconnect + phát hiện Ctrl+C
    while (g_running) {
        // Xử lý reconnect nếu connection_lost() đã set flag
        if (needsReconnect) {
            needsReconnect = false;
            std::cout << "[INFO] Thu reconnect sau " << RECONNECT_DELAY_SEC << "s...\n";
            std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SEC));

            if (!connectAndSetup(client, connOpts, commandTopic, statusTopic)) {
                // Reconnect thất bại, thử lại vòng tiếp
                needsReconnect = true;
                continue;
            }
        }

        device.simulate();

        std::string payload = device.getTypeName()  + ","
            + device.getId()                         + ","
            + device.getName()                       + ","
            + (device.isOn() ? "ON" : "OFF")         + ","
            + std::to_string(device.getBatteryLevel()) + ","
            + device.serializeExtra();

        try {
            auto msg = mqtt::make_message(dataTopic, payload);
            msg->set_qos(1);
            client.publish(msg)->wait();
            std::cout << "[" << id << "] Publish: " << payload << "\n";
        } catch (const mqtt::exception& e) {
            // publish thất bại -> mất kết nối, trigger reconnect vòng tiếp
            std::cerr << "[WARN] Publish that bai: " << e.what() << "\n";
            needsReconnect = true;
        }

        // sleep chia nhỏ thành nhiều đoạn 100ms để phát hiện Ctrl+C nhanh hơn,
        // thay vì sleep 2s liên tục (sẽ phải đợi đến 2s mới thoát).
        for (int i = 0; i < 20 && g_running && !needsReconnect; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Thoát sạch: publish OFFLINE rồi disconnect
    std::cout << "\n[INFO] Dang thoat sach...\n";
    try {
        auto offlineMsg = mqtt::make_message(statusTopic, "OFFLINE");
        offlineMsg->set_qos(1);
        offlineMsg->set_retained(true);
        client.publish(offlineMsg)->wait();
        client.disconnect()->wait();
        std::cout << "[INFO] Da disconnect. Thoat.\n";
    } catch (const mqtt::exception& e) {
        std::cerr << "[WARN] Loi khi thoat: " << e.what() << "\n";
    }

    return 0;
}
