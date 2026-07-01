#include "SmartLight.h"

SmartLight::SmartLight(const std::string& id, const std::string& name)
    : Device(id, name) {
}

void SmartLight::simulate() {
    if (!isOn()) return;
}

std::string SmartLight::getReadingInfo() const {
    return isOn() ? "Den dang: ON" : "Den dang: OFF";
}

std::string SmartLight::getTypeName() const {
    return "SmartLight";
}

std::string SmartLight::serializeExtra() const {
    return "";
}

void SmartLight::deserializeExtra(const std::string& /*data*/) {
    // Không có gì để khôi phục - tham số không dùng nên comment tên đi
    // để tránh warning "unused parameter"
}