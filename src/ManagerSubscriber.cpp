#include "Device.h"
#include "DeviceManager.h"
#include "DatabaseStorage.h"

#include <mqtt/async_client.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const std::string CLIENT_ID("manager_subscriber");
const std::string TOPIC_FILTER("iot/device/+/data");
const std::string STATUS_TOPIC_FILTER("iot/device/+/status");
const int RECONNECT_DELAY_SEC = 5;

std::atomic<bool> g_running{true};
void onSigInt(int) { g_running = false; }

class MyCallback : public virtual mqtt::callback {
private:
    DeviceManager&     deviceManager_;
    DatabaseStorage&   db_;
    std::atomic<bool>& needsReconnect_;

public:
    MyCallback(DeviceManager& dm, DatabaseStorage& db, std::atomic<bool>& reconnect)
        : deviceManager_(dm), db_(db), needsReconnect_(reconnect) {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string topic = msg->get_topic();
        std::string topicType = topic.substr(topic.find_last_of('/') + 1);

        if (topicType == "status") {
            std::string prefix = "iot/device/";
            std::string deviceId = topic.substr(
                prefix.size(),
                topic.size() - prefix.size() - std::string("/status").size());

            std::cout << "[STATUS] " << deviceId << " -> " << msg->to_string() << "\n";
            deviceManager_.setOnlineStatus(deviceId, msg->to_string() == "ONLINE");
            deviceManager_.printAllStatus();
            return;
        }

        std::istringstream iss(msg->to_string());
        std::string typeName, id, name, statusStr, batteryStr, extra;
        std::getline(iss, typeName,  ',');
        std::getline(iss, id,        ',');
        std::getline(iss, name,      ',');
        std::getline(iss, statusStr, ',');
        std::getline(iss, batteryStr,',');
        std::getline(iss, extra);

        deviceManager_.updateOrCreateDevice(
            typeName, id, name, statusStr, std::stoi(batteryStr), extra);

        if (Device* d = deviceManager_.findDevice(id)) {
            db_.upsertDevice(*d);
            db_.insertReading(*d);
        }

        deviceManager_.printAllStatus();
    }

    void connection_lost(const std::string& cause) override {
        std::cout << "[WARN] Mat ket noi broker: "
                  << (cause.empty() ? "(khong ro nguyen nhan)" : cause) << "\n";
        // Chỉ set flag, KHÔNG gọi connect() trực tiếp ở đây (deadlock risk)
        needsReconnect_ = true;
    }
};

// Helper: connect + subscribe cả 2 topic — dùng cho cả lần đầu lẫn reconnect
bool connectAndSubscribe(mqtt::async_client& client,
                          const mqtt::connect_options& connOpts) {
    try {
        client.connect(connOpts)->wait();
        client.subscribe(TOPIC_FILTER,       1)->wait();
        client.subscribe(STATUS_TOPIC_FILTER,1)->wait();
        std::cout << "[INFO] Ket noi thanh cong, dang lang nghe...\n";
        return true;
    } catch (const mqtt::exception& e) {
        std::cerr << "[ERROR] Ket noi that bai: " << e.what() << "\n";
        return false;
    }
}

int main() {
    std::signal(SIGINT, onSigInt);

    DeviceManager   manager;
    DatabaseStorage db("iot_manager.db");
    db.initSchema();

    std::atomic<bool> needsReconnect{false};

    mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);
    MyCallback cb(manager, db, needsReconnect);
    client.set_callback(cb);

    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);

    // Lần connect đầu
    while (g_running && !connectAndSubscribe(client, connOpts)) {
        std::cout << "[INFO] Thu lai sau " << RECONNECT_DELAY_SEC << "s...\n";
        std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SEC));
    }

    // Thread riêng theo dõi needsReconnect — độc lập với vòng đọc input.
    // Lý do cần tách: std::getline() BLOCK thread chính vô thời hạn khi
    // chờ người dùng gõ. Nếu để reconnect logic trong vòng getline(), nó
    // sẽ không bao giờ chạy trừ khi người dùng gõ Enter → broker mất kết
    // nối nhưng manager không bao giờ tự reconnect được.
    std::thread reconnectThread([&]() {
        while (g_running) {
            if (needsReconnect) {
                needsReconnect = false;
                std::cout << "[INFO] Thu reconnect sau "
                          << RECONNECT_DELAY_SEC << "s...\n";
                std::this_thread::sleep_for(
                    std::chrono::seconds(RECONNECT_DELAY_SEC));

                if (g_running && !connectAndSubscribe(client, connOpts)) {
                    needsReconnect = true; // thất bại, thử lại vòng tiếp
                }
            }
            // Poll mỗi 200ms — đủ nhanh để phát hiện mất kết nối,
            // không tốn CPU (khác với busy-wait không có sleep).
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    std::cout << "Go lenh: <ID> <ON|OFF>  (vi du: TEMP_01 OFF)\n";
    std::cout << "Go 'quit' hoac Ctrl+C de thoat.\n";

    std::string line;
    while (g_running && std::getline(std::cin, line)) {
        if (!g_running || line == "quit") break;
        if (line.empty()) continue;

        // Không xử lý reconnect ở đây nữa — reconnectThread lo việc đó
        std::istringstream iss(line);
        std::string deviceId, command;
        iss >> deviceId >> command;

        if (deviceId.empty() || command.empty()) {
            std::cout << "Cu phap sai. Vi du: TEMP_01 OFF\n"; continue;
        }
        if (command != "ON" && command != "OFF") {
            std::cout << "Lenh phai la ON hoac OFF.\n"; continue;
        }

        try {
            std::string commandTopic = "iot/device/" + deviceId + "/command";
            auto msg = mqtt::make_message(commandTopic, command);
            msg->set_qos(1);
            msg->set_retained(true);
            client.publish(msg)->wait();
            std::cout << "Da gui lenh [" << command << "] toi ["
                      << deviceId << "]\n";
        } catch (const mqtt::exception& e) {
            std::cerr << "[ERROR] Gui lenh that bai: " << e.what() << "\n";
            needsReconnect = true;
        }
    }

    // Thoát sạch: dừng reconnect thread trước, rồi mới disconnect
    g_running = false;
    reconnectThread.join(); // chờ thread kết thúc, không để nó chạy lơ lửng

    std::cout << "\n[INFO] Dang thoat...\n";
    try {
        client.disconnect()->wait();
        std::cout << "[INFO] Da disconnect. Thoat.\n";
    } catch (const mqtt::exception& e) {
        std::cerr << "[WARN] Loi khi disconnect: " << e.what() << "\n";
    }

    return 0;
}