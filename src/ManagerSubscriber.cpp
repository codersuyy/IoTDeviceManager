#include "Device.h"
#include "DeviceManager.h"

#include <mqtt/async_client.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const std::string CLIENT_ID("manager_subscriber");
const std::string TOPIC_FILTER("iot/device/+/data");
const std::string STATUS_TOPIC("iot/device/+/status");

class MyCallback : public virtual mqtt::callback {
private:
    DeviceManager& deviceManager_;

public:
    MyCallback(DeviceManager& deviceManager) : deviceManager_(deviceManager) {}

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string topic = msg->get_topic();
        auto lastSlash = topic.find_last_of('/');
        std::string topicType = topic.substr(lastSlash + 1); // "data" hoặc "status"

        if (topicType == "status") {
            std::string prefix = "iot/device/";
            std::string deviceId = topic.substr(
                prefix.size(), topic.size() - prefix.size() - std::string("/status").size());

            std::cout << "[STATUS] " << deviceId << " -> " << msg->to_string() << "\n";
            deviceManager_.setOnlineStatus(deviceId, msg->to_string() == "ONLINE");

            deviceManager_.printAllStatus();
            return;
        }

        std::istringstream iss(msg->to_string());
        std::string typeName, id, name, statusStr, batteryStr, extra;

        std::getline(iss, typeName, ',');
        std::getline(iss, id, ',');
        std::getline(iss, name, ',');
        std::getline(iss, statusStr, ',');
        std::getline(iss, batteryStr, ',');
        std::getline(iss, extra);

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

        client.subscribe(TOPIC_FILTER, 1)->wait();
        client.subscribe(STATUS_TOPIC, 1)->wait();

        std::cout << "Dang lang nghe...\n";
        std::cout << "Go lenh theo cu phap: <ID> <ON|OFF>   (vi du: TEMP_01 OFF)\n";
        std::cout << "Go 'quit' de thoat.\n";

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "quit") break;
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string deviceId, command;
            iss >> deviceId >> command;

            if (deviceId.empty() || command.empty()) {
                std::cout << "Cu phap sai. Vi du: TEMP_01 OFF\n";
                continue;
            }
            if (command != "ON" && command != "OFF") {
                std::cout << "Lenh phai la ON hoac OFF.\n";
                continue;
            }

            std::string commandTopic = "iot/device/" + deviceId + "/command";
            auto msg = mqtt::make_message(commandTopic, command);
            msg->set_qos(1);
            msg->set_retained(true);

            client.publish(msg)->wait();
            std::cout << "Da gui lenh [" << command << "] toi thiet bi [" << deviceId << "]\n";
        }

        client.disconnect()->wait();

    } catch (const mqtt::exception& exc) {
        std::cerr << "Loi MQTT: " << exc.what() << "\n";
        return 1;
    }

    return 0;
}