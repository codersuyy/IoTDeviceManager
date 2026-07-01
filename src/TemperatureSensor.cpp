#include "TemperatureSensor.h"
#include <cstdlib>   // rand()
#include <sstream>   // std::ostringstream để format chuỗi

TemperatureSensor::TemperatureSensor(const std::string& id, const std::string& name,
                                      double minTemp, double maxTemp)
    : Device(id, name), currentTemp_(minTemp), minTemp_(minTemp), maxTemp_(maxTemp) {
}

void TemperatureSensor::simulate() {
    if (!isOn()) return;

    double range = maxTemp_ - minTemp_;
    currentTemp_ = minTemp_ + static_cast<double>(rand() % 1000) / 1000.0 * range;

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
    std::ostringstream oss;
    oss << currentTemp_ << ";" << minTemp_ << ";" << maxTemp_;
    return oss.str();
}

void TemperatureSensor::deserializeExtra(const std::string& data) {
    std::istringstream iss(data);
    std::string token;

    std::getline(iss, token, ';');
    currentTemp_ = std::stod(token);

    std::getline(iss, token, ';');
    minTemp_ = std::stod(token);

    std::getline(iss, token, ';');
    maxTemp_ = std::stod(token);
}