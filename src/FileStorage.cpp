#include "FileStorage.h"
#include "TemperatureSensor.h"
#include "SmartLight.h"
#include "SecurityCamera.h"

#include <fstream>   // std::ofstream, std::ifstream
#include <sstream>   // std::ostringstream, std::istringstream
#include <iostream>

bool FileStorage::save(const std::string& filename,
                        const std::vector<std::unique_ptr<Device>>& devices) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Loi: khong the mo file de luu: " << filename << "\n";
        return false;
    }
    file << "TYPE,ID,NAME,STATUS,BATTERY,EXTRA\n";

    for (const auto& device : devices) {
        file << device->getTypeName() << ","
             << device->getId() << ","
             << device->getName() << ","
             << (device->isOn() ? "ON" : "OFF") << ","
             << device->getBatteryLevel() << ","
             << device->serializeExtra() << "\n";
    }
    file.close();
    return true;
}
    
std::unique_ptr<Device> FileStorage::createDeviceByType(
    const std::string& typeName,
    const std::string& id,
    const std::string& name) {

    if (typeName == "TemperatureSensor") {
        return std::make_unique<TemperatureSensor>(id, name);
    } else if (typeName == "SmartLight") {
        return std::make_unique<SmartLight>(id, name);
    } else if (typeName == "SecurityCamera") {
        return std::make_unique<SecurityCamera>(id, name);
    }

    std::cerr << "Canh bao: khong nhan dien duoc loai thiet bi '" 
               << typeName << "', bo qua dong nay.\n";
    return nullptr;
}

std::vector<std::unique_ptr<Device>> FileStorage::load(const std::string& filename) {
    std::vector<std::unique_ptr<Device>> devices;

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Khong tim thay file: " << filename 
                   << " (co the day la lan dau chay, chua co du lieu cu)\n";
        return devices;
    }

    std::string line;
    bool isFirstLine = true;
    while (std::getline(file, line)) {
        if (isFirstLine) {
            isFirstLine = false;
            continue;
        }

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string typeName, id, name, statusStr, batteryStr, extra;

        std::getline(iss, typeName, ',');
        std::getline(iss, id, ',');
        std::getline(iss, name, ',');
        std::getline(iss, statusStr, ',');
        std::getline(iss, batteryStr, ',');
        std::getline(iss, extra);

        auto device = createDeviceByType(typeName, id, name);
        if (!device) continue;

        device->setStatus(statusStr == "ON" ? DeviceStatus::ON : DeviceStatus::OFF);
        device->setBatteryLevel(std::stoi(batteryStr));
        device->deserializeExtra(extra);

        devices.push_back(std::move(device));
    }

    return devices;
}