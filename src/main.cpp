#include "Device.h"
#include "TemperatureSensor.h"
#include "SmartLight.h"
#include "SecurityCamera.h"
#include "DeviceManager.h"

#include <iostream>
#include <memory>
#include <thread>   // std::this_thread::sleep_for
#include <chrono>   // std::chrono::seconds

int main() {
    DeviceManager manager;

    // make_unique: cách an toàn để tạo unique_ptr, tránh viết tay "new"
    // (an toàn hơn vì nếu constructor throw exception, không bị leak)
    manager.addDevice(std::make_unique<TemperatureSensor>(
        "TEMP_01", "Cam bien nhiet do phong khach", 20.0, 35.0));

    manager.addDevice(std::make_unique<TemperatureSensor>(
        "TEMP_02", "Cam bien nhiet do phong ngu", 18.0, 28.0));

    manager.addDevice(std::make_unique<SmartLight>(
        "LIGHT_01", "Den phong khach"));

    manager.addDevice(std::make_unique<SecurityCamera>(
        "CAM_01", "Camera an ninh phong khach"));

    std::cout << "Tong so thiet bi: " << manager.getDeviceCount() << "\n\n";

    // Bật tất cả thiết bị lên để chúng bắt đầu "hoạt động"
    manager.turnOnDevice("TEMP_01");
    manager.turnOnDevice("TEMP_02");
    manager.turnOnDevice("LIGHT_01");
    manager.turnOnDevice("CAM_01");

    // Vòng lặp mô phỏng: mỗi giây, tất cả thiết bị "đo" 1 lần, rồi in trạng thái
    const int SIMULATION_ROUNDS = 5;
    for (int round = 1; round <= SIMULATION_ROUNDS; ++round) {
        std::cout << "--- Vong mo phong #" << round << " ---\n";
        manager.simulateAll();
        manager.printAllStatus();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Thử tắt 1 thiết bị và xem trạng thái cập nhật
    std::cout << "\n>> Tat den phong khach...\n";
    manager.turnOffDevice("LIGHT_01");
    manager.simulateAll();
    manager.printAllStatus();

    // Thử xóa 1 thiết bị
    std::cout << "\n>> Xoa cam bien TEMP_02...\n";
    manager.removeDevice("TEMP_02");
    manager.printAllStatus();

    // ----- Demo luu/doc file -----
    const std::string SAVE_FILE = "devices.csv";

    std::cout << "\n>> Luu trang thai ra file '" << SAVE_FILE << "'...\n";
    if (manager.saveToFile(SAVE_FILE)) {
        std::cout << "Da luu thanh cong.\n";
    }

    std::cout << "\n>> Tao 1 DeviceManager MOI va doc lai tu file...\n";
    DeviceManager manager2;
    manager2.loadFromFile(SAVE_FILE);
    std::cout << "Doc duoc " << manager2.getDeviceCount() << " thiet bi tu file:\n";
    manager2.printAllStatus();

    return 0;
}