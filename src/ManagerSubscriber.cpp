#include "Device.h"
#include "DeviceManager.h"

#include <mqtt/async_client.h>
#include <iostream>
#include <sstream>  // std::istringstream - BỊ THIẾU trong bản trước, gây lỗi compile
#include <thread>
#include <chrono>

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const std::string CLIENT_ID("manager_subscriber");
const std::string TOPIC_FILTER("iot/device/+/data");

class MyCallback : public virtual mqtt::callback {
private:
    // Dùng tham chiếu (DeviceManager&), KHÔNG cần shared_ptr ở đây,
    // vì callback chỉ "mượn dùng" manager trong suốt vòng đời main(),
    // không sở hữu nó. shared_ptr chỉ cần khi nhiều nơi cùng giữ quyền
    // sở hữu 1 object - ở đây main() là chủ duy nhất.
    DeviceManager& deviceManager_;

public:
    MyCallback(DeviceManager& deviceManager) : deviceManager_(deviceManager) {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "Nhan duoc message tren topic [" << msg->get_topic()
                   << "]: " << msg->to_string() << "\n";

        std::istringstream iss(msg->to_string());
        std::string typeName, id, name, statusStr, batteryStr, extra;

        std::getline(iss, typeName, ',');
        std::getline(iss, id, ',');
        std::getline(iss, name, ',');
        std::getline(iss, statusStr, ',');
        std::getline(iss, batteryStr, ',');
        std::getline(iss, extra);

        // 1 hàm duy nhất lo cả việc "tìm rồi update" hoặc "tạo mới" -
        // không viết lại if/else loại thiết bị ở đây.
        deviceManager_.updateOrCreateDevice(
            typeName, id, name, statusStr, std::stoi(batteryStr), extra);

        deviceManager_.printAllStatus();
    }

    void connection_lost(const std::string& cause) override {
        std::cout << "Mat ket noi: " << cause << "\n";
    }
};

int main() {
    DeviceManager manager;

    mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);

    MyCallback cb(manager);
    client.set_callback(cb);

    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);

    try {
        std::cout << "Dang ket noi toi broker...\n";
        client.connect(connOpts)->wait();

        std::cout << "Dang subscribe topic: " << TOPIC_FILTER << "\n";
        client.subscribe(TOPIC_FILTER, 1)->wait();

        std::cout << "Dang lang nghe... (Ctrl+C de thoat)\n";

        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const mqtt::exception& exc) {
        std::cerr << "Loi MQTT: " << exc.what() << "\n";
        return 1;
    }

    return 0;
}