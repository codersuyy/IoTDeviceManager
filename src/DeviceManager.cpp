#include "DeviceManager.h"
#include "FileStorage.h"
#include <iostream>
#include <algorithm> // std::find_if, std::remove_if

void DeviceManager::addDevice(std::unique_ptr<Device> device) {
    // std::move bắt buộc: unique_ptr không có copy constructor,
    // chỉ có move constructor. "device" tham số sẽ thành rỗng (nullptr)
    // sau dòng này, vì quyền sở hữu đã chuyển vào devices_.
    devices_.push_back(std::move(device));
}

Device* DeviceManager::findDevice(const std::string& id) {
    // std::find_if duyệt qua vector, dùng lambda để so sánh điều kiện
    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&id](const std::unique_ptr<Device>& d) {
            return d->getId() == id;
        });

    if (it == devices_.end()) {
        return nullptr; // không tìm thấy
    }
    return it->get();
}

bool DeviceManager::removeDevice(const std::string& id) {
    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&id](const std::unique_ptr<Device>& d) {
            return d->getId() == id;
        });

    if (it == devices_.end()) {
        return false;
    }
    devices_.erase(it);
    return true;
}

void DeviceManager::simulateAll() {
    for (auto& device : devices_) {
        device->simulate();
    }
}

void DeviceManager::printAllStatus() const {
    std::cout << "===== TRANG THAI " << devices_.size() << " THIET BI =====\n";
    for (const auto& device : devices_) {
        std::string id = device->getId();

        // Tra map riêng để biết trạng thái KẾT NỐI - độc lập với
        // device->isOn() (trạng thái CHỨC NĂNG). Nếu chưa từng nhận
        // message status nào cho id này, coi như "Unknown".
        std::string connState = "Unknown";
        auto it = onlineStatus_.find(id);
        if (it != onlineStatus_.end()) {
            connState = it->second ? "ONLINE" : "OFFLINE";
        }

        std::cout << "[" << id << "] " << device->getName()
                   << " | Ket noi: " << connState
                   << " | Chuc nang: " << (device->isOn() ? "ON " : "OFF")
                   << " | Pin: " << device->getBatteryLevel() << "%"
                   << " | " << device->getReadingInfo() << "\n";
    }
    std::cout << "================================\n";
}

void DeviceManager::setOnlineStatus(const std::string& id, bool online) {
    onlineStatus_[id] = online;
}

bool DeviceManager::isOnline(const std::string& id) const {
    auto it = onlineStatus_.find(id);
    return it != onlineStatus_.end() && it->second;
}

void DeviceManager::turnOnDevice(const std::string& id) {
    Device* d = findDevice(id);
    if (d) d->turnOn();
}

void DeviceManager::turnOffDevice(const std::string& id) {
    Device* d = findDevice(id);
    if (d) d->turnOff();
}

size_t DeviceManager::getDeviceCount() const {
    return devices_.size();
}

bool DeviceManager::saveToFile(const std::string& filename) const {
    return FileStorage::save(filename, devices_);
}

void DeviceManager::loadFromFile(const std::string& filename) {
    // Load ra 1 vector mới, rồi "move" từng cái vào devices_.
    // Dùng std::move(loaded) trực tiếp gán cho devices_ là cách gọn nhất,
    // vì FileStorage::load() trả về 1 vector tạm (temporary) - ta chiếm
    // luôn quyền sở hữu, không cần copy từng phần tử.
    devices_ = FileStorage::load(filename);
}

void DeviceManager::updateOrCreateDevice(const std::string& typeName,
                                          const std::string& id,
                                          const std::string& name,
                                          const std::string& statusStr,
                                          int battery,
                                          const std::string& extra) {
    Device* existing = findDevice(id);

    if (existing != nullptr) {
        existing->setStatus(statusStr == "ON" ? DeviceStatus::ON : DeviceStatus::OFF);
        existing->setBatteryLevel(battery);
        existing->deserializeExtra(extra);
        return;
    }

    auto newDevice = FileStorage::createDeviceByType(typeName, id, name);
    if (newDevice == nullptr) {
        return; // loại không nhận diện được, FileStorage đã tự in cảnh báo
    }

    newDevice->setStatus(statusStr == "ON" ? DeviceStatus::ON : DeviceStatus::OFF);
    newDevice->setBatteryLevel(battery);
    newDevice->deserializeExtra(extra);

    addDevice(std::move(newDevice));
}