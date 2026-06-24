#include "Device.h"
#include <algorithm> // std::max, std::min

Device::Device(const std::string& id, const std::string& name)
    : id_(id), name_(name), status_(DeviceStatus::OFF), batteryLevel_(100) {
}

void Device::turnOn() {
    status_ = DeviceStatus::ON;
}

void Device::turnOff() {
    status_ = DeviceStatus::OFF;
}

bool Device::isOn() const {
    return status_ == DeviceStatus::ON;
}

std::string Device::getId() const {
    return id_;
}

std::string Device::getName() const {
    return name_;
}

int Device::getBatteryLevel() const {
    return batteryLevel_;
}

DeviceStatus Device::getStatus() const {
    return status_;
}

void Device::drainBattery(int amount) {
    // std::max đảm bảo pin không bị âm dưới 0
    batteryLevel_ = std::max(0, batteryLevel_ - amount);
}

void Device::setStatus(DeviceStatus status) {
    status_ = status;
}

void Device::setBatteryLevel(int level) {
    batteryLevel_ = std::max(0, std::min(100, level));
}