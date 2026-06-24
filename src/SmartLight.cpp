#include "SmartLight.h"

SmartLight::SmartLight(const std::string& id, const std::string& name)
    : Device(id, name) {
}

void SmartLight::simulate() {
    if (!isOn()) return;
    // Đèn bật thì tốn pin ít hơn sensor (giả định đèn cắm điện, ít tốn pin dự phòng)
    drainBattery(0); // ở đây để 0, sau này có thể mô phỏng pin dự phòng riêng
}

std::string SmartLight::getReadingInfo() const {
    return isOn() ? "Den dang: ON" : "Den dang: OFF";
}

std::string SmartLight::getTypeName() const {
    return "SmartLight";
}

std::string SmartLight::serializeExtra() const {
    return ""; // SmartLight không có dữ liệu riêng nào cần lưu thêm
}

void SmartLight::deserializeExtra(const std::string& /*data*/) {
    // Không có gì để khôi phục - tham số không dùng nên comment tên đi
    // để tránh warning "unused parameter"
}