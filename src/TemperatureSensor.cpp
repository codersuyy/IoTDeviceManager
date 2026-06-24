#include "TemperatureSensor.h"
#include <cstdlib>   // rand()
#include <sstream>   // std::ostringstream để format chuỗi

TemperatureSensor::TemperatureSensor(const std::string& id, const std::string& name,
                                      double minTemp, double maxTemp)
    // Gọi constructor của lớp cha Device trước, bắt buộc phải làm vậy
    // vì Device không có constructor mặc định (không có tham số).
    : Device(id, name), currentTemp_(minTemp), minTemp_(minTemp), maxTemp_(maxTemp) {
}

void TemperatureSensor::simulate() {
    if (!isOn()) return; // thiết bị tắt thì không đo

    // Mô phỏng nhiệt độ dao động ngẫu nhiên trong khoảng [minTemp_, maxTemp_]
    double range = maxTemp_ - minTemp_;
    currentTemp_ = minTemp_ + static_cast<double>(rand() % 1000) / 1000.0 * range;

    // Mỗi lần đo tốn một ít pin
    drainBattery(1);
}

std::string TemperatureSensor::getReadingInfo() const {
    std::ostringstream oss;
    oss << "Nhiet do: " << currentTemp_ << " C";
    return oss.str();
}

double TemperatureSensor::getCurrentTemp() const {
    return currentTemp_;
}

std::string TemperatureSensor::getTypeName() const {
    return "TemperatureSensor";
}

std::string TemperatureSensor::serializeExtra() const {
    // Dùng ";" để ngăn cách các giá trị bên trong 1 cột EXTRA_DATA
    // (vì dấu "," đã dùng để ngăn cách các cột chính trong file CSV)
    std::ostringstream oss;
    oss << currentTemp_ << ";" << minTemp_ << ";" << maxTemp_;
    return oss.str();
}

void TemperatureSensor::deserializeExtra(const std::string& data) {
    // Tách chuỗi "25.5;20.0;35.0" thành 3 số double, dựa trên dấu ";"
    std::istringstream iss(data);
    std::string token;

    std::getline(iss, token, ';');
    currentTemp_ = std::stod(token);

    std::getline(iss, token, ';');
    minTemp_ = std::stod(token);

    std::getline(iss, token, ';');
    maxTemp_ = std::stod(token);
}