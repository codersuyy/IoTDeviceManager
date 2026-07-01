#include "SecurityCamera.h"
#include <cstdlib>   // rand()
#include <sstream>   // std::ostringstream, std::istringstream

SecurityCamera::SecurityCamera(const std::string& id, const std::string& name)
    : Device(id, name), isRecording_(false), resolution_("1080p") {
}

void SecurityCamera::simulate() {
    if (!isOn()) return;
    isRecording_ = (rand() % 2 == 0);

    drainBattery(2);
}

std::string SecurityCamera::getReadingInfo() const {
    std::ostringstream oss;
    oss << "Resolution: " << resolution_
        << ", Recording: " << (isRecording_ ? "YES" : "NO");
    return oss.str();
}

std::string SecurityCamera::getTypeName() const {
    return "SecurityCamera";
}

std::string SecurityCamera::serializeExtra() const {
    std::ostringstream oss;
    oss << (isRecording_ ? "1" : "0") << ";" << resolution_;
    return oss.str();
}

void SecurityCamera::deserializeExtra(const std::string& data) {
    std::istringstream iss(data);
    std::string token;

    std::getline(iss, token, ';');
    isRecording_ = (token == "1");

    std::getline(iss, token, ';');
    resolution_ = token;
}