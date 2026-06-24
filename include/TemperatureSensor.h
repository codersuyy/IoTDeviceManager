#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include "Device.h"

// Kế thừa public từ Device -> TemperatureSensor LÀ MỘT Device
// (is-a relationship), nên có thể dùng Device* để trỏ tới nó.
class TemperatureSensor : public Device {
private:
    double currentTemp_;   // nhiệt độ hiện tại (°C)
    double minTemp_;       // khoảng giá trị mô phỏng ngẫu nhiên
    double maxTemp_;

public:
    TemperatureSensor(const std::string& id, const std::string& name,
                       double minTemp = 18.0, double maxTemp = 35.0);

    // override: bắt buộc phải có vì Device::simulate() là pure virtual.
    // Từ khóa "override" không bắt buộc về cú pháp, nhưng NÊN dùng để
    // compiler kiểm tra giúp bạn: nếu bạn gõ sai tên hàm (ví dụ Simulate
    // viết hoa), compiler sẽ báo lỗi ngay, thay vì âm thầm tạo ra 1 hàm mới.
    void simulate() override;
    std::string getReadingInfo() const override;

    std::string getTypeName() const override;
    std::string serializeExtra() const override;
    void deserializeExtra(const std::string& data) override;

    double getCurrentTemp() const;
};

#endif // TEMPERATURE_SENSOR_H