#ifndef SECURITY_CAMERA_H
#define SECURITY_CAMERA_H

#include "Device.h"

class SecurityCamera : public Device {
private:
    bool isRecording_; 
    std::string resolution_;
public:
    SecurityCamera(const std::string& id, const std::string& name);

    void simulate() override;
    std::string getReadingInfo() const override;

    std::string getTypeName() const override;
    std::string serializeExtra() const override;
    void deserializeExtra(const std::string& data) override;

};

#endif // SECURITY_CAMERA_H