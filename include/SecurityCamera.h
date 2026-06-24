#ifndef SECURITY_CAMERA_H
#define SECURITY_CAMERA_H

#include "Device.h"

class SecurityCamera : public Device {
private:
    bool isRecording_; // trạng thái ghi hình (ON/OFF)
    std::string resolution_; // độ phân giải video (ví dụ "1080p", "720p")
public:
    SecurityCamera(const std::string& id, const std::string& description);

    void simulate() override;
    std::string getReadingInfo() const override;

    std::string getTypeName() const override;
    std::string serializeExtra() const override;
    void deserializeExtra(const std::string& data) override;

};

#endif // SECURITY_CAMERA_H