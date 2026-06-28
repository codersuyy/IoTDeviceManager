#include <mqtt/async_client.h>
#include <iostream>
#include <thread>
#include <chrono>

#include "Device.h"
#include "TemperatureSensor.h"
#include "SmartLight.h"
#include "SecurityCamera.h"

const std::string SERVER_ADDRESS("tcp://localhost:1883");
const std::string CLIENT_ID("device_publisher");

int main(){
    mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);
    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);

    try{
        // Tạo một TemperatureSensor
        TemperatureSensor temp("TEMP_01", "Cam bien nhiet do phong khach");
        temp.turnOn();
    
        const std::string TOPIC("iot/device/" + temp.getId() + "/data");

        std::cout << "Dang ket noi toi broker: " << SERVER_ADDRESS << "...\n";
        client.connect(connOpts)->wait();
        std::cout << "Ket noi thanh cong!\n";

        for (int i = 1; i <= 5; ++i) {

            temp.simulate(); // Cập nhật trạng thái của cảm biến nhiệt độ

            // Tạo payload để gửi
            std::string payload = temp.getTypeName() + ","
             + temp.getId() + ","
             + temp.getName() + ","
             + (temp.isOn() ? "ON" : "OFF") + ","
             + std::to_string(temp.getBatteryLevel()) + ","
             + temp.serializeExtra();

            auto msg = mqtt::make_message(TOPIC, payload);
            msg->set_qos(1); // QoS 1: đảm bảo message tới ít nhất 1 lần

            std::cout << "Publish: " << payload << "\n";
            client.publish(msg)->wait();

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        client.disconnect()->wait();
        std::cout << "Da disconnect.\n";

    } catch (const mqtt::exception& exc) {
        std::cerr << "Loi MQTT: " << exc.what() << "\n";
        return 1;
    }
}