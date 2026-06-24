#ifndef DEVICE_H
#define DEVICE_H

#include <string>

// Trạng thái thiết bị: ON hoặc OFF
enum class DeviceStatus {
    ON,
    OFF
};

// Lớp cơ sở trừu tượng (abstract base class) cho mọi thiết bị IoT.
// Ta dùng "abstract" vì mỗi loại thiết bị (sensor, đèn, camera...)
// sẽ có cách hoạt động riêng -> bắt buộc lớp con phải tự định nghĩa.
class Device {
protected:
    std::string id_;       // mã định danh thiết bị, ví dụ "TEMP_01"
    std::string name_;     // tên dễ hiểu, ví dụ "Cảm biến nhiệt độ phòng khách"
    DeviceStatus status_;
    int batteryLevel_;     // 0-100 (%)

public:
    // Constructor: khởi tạo các thuộc tính chung
    Device(const std::string& id, const std::string& name);

    // Destructor ảo (virtual) - RẤT QUAN TRỌNG khi có class con
    // Nếu không có "virtual", khi xóa con trỏ Device* trỏ tới object con,
    // destructor của con sẽ KHÔNG được gọi -> rò rỉ bộ nhớ (memory leak).
    virtual ~Device() = default;

    // Hàm thuần ảo (pure virtual) - lớp con BẮT BUỘC phải override
    // Mô phỏng việc thiết bị "đọc dữ liệu" hoặc "cập nhật trạng thái"
    virtual void simulate() = 0;

    // Hàm thuần ảo - in thông tin riêng của từng loại thiết bị
    virtual std::string getReadingInfo() const = 0;

    // --- Hỗ trợ lưu/đọc file (serialization) ---
    // Trả về tên loại thiết bị, dùng để biết khi đọc file nên tạo lại
    // loại class nào (ví dụ "TemperatureSensor", "SmartLight")
    virtual std::string getTypeName() const = 0;

    // Trả về dữ liệu riêng của từng loại dưới dạng 1 chuỗi (để ghi vào file).
    // Mỗi loại tự quyết định format bên trong chuỗi này.
    virtual std::string serializeExtra() const = 0;

    // Nhận 1 chuỗi (đọc từ file) và khôi phục lại dữ liệu riêng.
    // Đây là "ngược lại" của serializeExtra().
    virtual void deserializeExtra(const std::string& data) = 0;

    // Các hàm dùng chung, không cần override
    void turnOn();
    void turnOff();
    bool isOn() const;

    std::string getId() const;
    std::string getName() const;
    int getBatteryLevel() const;
    DeviceStatus getStatus() const;

    // Giảm pin một chút mỗi lần simulate (mô phỏng tiêu hao năng lượng)
    void drainBattery(int amount);

    // Setter dùng riêng cho việc load lại trạng thái từ file
    // (khác với turnOn/turnOff vì đây set trực tiếp, không qua logic nghiệp vụ)
    void setStatus(DeviceStatus status);
    void setBatteryLevel(int level);
};

#endif // DEVICE_H