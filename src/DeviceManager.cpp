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
    // it->get() : lấy con trỏ thường (Device*) ra từ unique_ptr,
    // KHÔNG chuyển quyền sở hữu, chỉ "mượn" để dùng tạm.
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
    devices_.erase(it); // unique_ptr tự động gọi delete object bên trong
    return true;
}

void DeviceManager::simulateAll() {
    for (auto& device : devices_) {
        device->simulate(); // gọi đa hình - không cần biết loại cụ thể
    }
}

void DeviceManager::printAllStatus() const {
    std::cout << "===== TRANG THAI " << devices_.size() << " THIET BI =====\n";
    for (const auto& device : devices_) {
        std::cout << "[" << device->getId() << "] " << device->getName()
                   << " | " << (device->isOn() ? "ON " : "OFF")
                   << " | Pin: " << device->getBatteryLevel() << "%"
                   << " | " << device->getReadingInfo() << "\n";
    }
    std::cout << "================================\n";
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
    Device* existingDevice = findDevice(id);
    if (existingDevice) {
        // Cập nhật trạng thái của thiết bị hiện có
        existingDevice->setStatus(statusStr == "ON" ? DeviceStatus::ON : DeviceStatus::OFF);
        existingDevice->setBatteryLevel(battery);
        existingDevice->deserializeExtra(extra);
    } else {
        // Tạo thiết bị mới dựa trên typeName
        auto newDevice = FileStorage::createDeviceByType(typeName, id, name);
        if (newDevice) {
            newDevice->setStatus(statusStr == "ON" ? DeviceStatus::ON : DeviceStatus::OFF);
            newDevice->setBatteryLevel(battery);
            newDevice->deserializeExtra(extra);
            addDevice(std::move(newDevice));
        } else {
            std::cerr << "Khong the tao thiet bi moi: loai khong hop le (" << typeName << ")\n";
        }
    }
}