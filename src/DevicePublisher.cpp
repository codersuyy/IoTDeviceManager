#include <mqtt/async_client.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <ctime>   // time() cho srand()

#include "Device.h"
#include "TemperatureSensor.h"
#include "SmartLight.h"
#include "SecurityCamera.h"

const std::string SERVER_ADDRESS("tcp://localhost:1883");

class CommandCallback : public virtual mqtt::callback {
private:
    Device& device_;

public:
    explicit CommandCallback(Device& device) : device_(device) {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string payload = msg->to_string();
        std::cout << "[COMMAND] Nhan duoc lenh: " << payload << "\n";

        if (payload == "ON") {
            device_.turnOn();
            std::cout << "-> Da BAT thiet bi theo lenh tu xa.\n";
        } else if (payload == "OFF") {
            device_.turnOff();
            std::cout << "-> Da TAT thiet bi theo lenh tu xa.\n";
        } else {
            std::cout << "-> Lenh khong hop le, bo qua.\n";
        }
    }

    void connection_lost(const std::string& cause) override {
        std::cout << "Mat ket noi: " << cause << "\n";
    }
};

std::unique_ptr<Device> createDevice(const std::string& type,
                                      const std::string& id,
                                      const std::string& name) {
    if (type == "TemperatureSensor") {
        return std::make_unique<TemperatureSensor>(id, name);
    } else if (type == "SmartLight") {
        return std::make_unique<SmartLight>(id, name);
    } else if (type == "SecurityCamera") {
        return std::make_unique<SecurityCamera>(id, name);
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    // Seed ngẫu nhiên 1 lần duy nhất ở đầu chương trình - đảm bảo rand()
    // trong simulate() cho số khác nhau mỗi lần chạy, không lặp lại.
    srand(static_cast<unsigned>(time(nullptr)));
    // Cách dùng: ./device_publisher <Type> <ID> <Name>
    // Ví dụ:     ./device_publisher TemperatureSensor TEMP_01 "Cam bien phong khach"
    if (argc < 4) {
        std::cerr << "Cach dung: " << argv[0] << " <Type> <ID> <Name>\n"
                  << "Vi du: " << argv[0]
                  << " TemperatureSensor TEMP_01 \"Cam bien phong khach\"\n"
                  << "Cac Type ho tro: TemperatureSensor, SmartLight, SecurityCamera\n";
        return 1;
    }

    std::string type = argv[1];
    std::string id = argv[2];
    std::string name = argv[3];

    auto devicePtr = createDevice(type, id, name);
    if (!devicePtr) {
        std::cerr << "Loi: khong nhan dien duoc loai thiet bi '" << type << "'\n";
        return 1;
    }

    Device& device = *devicePtr;
    device.turnOn();

    std::string clientId = "device_publisher_" + id;

    const std::string DATA_TOPIC("iot/device/" + id + "/data");
    const std::string STATUS_TOPIC("iot/device/" + id + "/status");
    const std::string COMMAND_TOPIC("iot/device/" + id + "/command");

    try {
        mqtt::async_client client(SERVER_ADDRESS, clientId);

        CommandCallback cb(device);
        client.set_callback(cb);

        mqtt::message willMsg(STATUS_TOPIC, "OFFLINE", 1, true);
        mqtt::will_options willOpts(willMsg);

        mqtt::connect_options connOpts;
        connOpts.set_clean_session(true);
        connOpts.set_will(willOpts);

        std::cout << "[" << id << "] Dang ket noi toi broker...\n";
        client.connect(connOpts)->wait();
        std::cout << "[" << id << "] Ket noi thanh cong!\n";

        client.subscribe(COMMAND_TOPIC, 1)->wait();
        std::cout << "[" << id << "] Dang lang nghe lenh tren: " << COMMAND_TOPIC << "\n";

        auto onlineMsg = mqtt::make_message(STATUS_TOPIC, "ONLINE");
        onlineMsg->set_qos(1);
        onlineMsg->set_retained(true);
        client.publish(onlineMsg)->wait();
        while (true) {
            device.simulate();

            std::string payload = device.getTypeName() + ","
                + device.getId() + ","
                + device.getName() + ","
                + (device.isOn() ? "ON" : "OFF") + ","
                + std::to_string(device.getBatteryLevel()) + ","
                + device.serializeExtra();

            auto msg = mqtt::make_message(DATA_TOPIC, payload);
            msg->set_qos(1);

            std::cout << "[" << id << "] Publish: " << payload << "\n";
            client.publish(msg)->wait();

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

    } catch (const mqtt::exception& exc) {
        std::cerr << "Loi MQTT: " << exc.what() << "\n";
        return 1;
    }

    return 0;
}