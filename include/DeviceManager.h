#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "Device.h"
#include <vector>
#include <memory>   // std::unique_ptr
#include <string>

class DeviceManager {
private:
    // std::unique_ptr<Device>: con trỏ thông minh "sở hữu duy nhất" 1 object.
    // Vì sao dùng unique_ptr thay vì Device* thường?
    //  -> Khi vector bị xóa (hoặc object bị remove khỏi vector), unique_ptr
    //     TỰ ĐỘNG gọi delete cho ta -> không lo memory leak, không cần
    //     tự viết delete tay ở đâu cả.
    // Vì sao dùng vector<unique_ptr<Device>> mà không phải vector<Device>?
    //  -> Device là abstract class (có pure virtual) -> KHÔNG THỂ tạo
    //     object Device trực tiếp, và mỗi object trong vector có thể là
    //     1 trong nhiều loại con khác nhau (TemperatureSensor, SmartLight...)
    //     với kích thước khác nhau -> phải dùng con trỏ (polymorphism).
    std::vector<std::unique_ptr<Device>> devices_;

public:
    DeviceManager() = default;

    // std::move: vì unique_ptr KHÔNG cho phép copy (chỉ 1 chủ sở hữu),
    // ta phải "chuyển quyền sở hữu" từ bên gọi vào trong vector_.
    void addDevice(std::unique_ptr<Device> device);

    // Trả về con trỏ thường (Device*) để "xem/dùng tạm", không chuyển quyền
    // sở hữu. Trả nullptr nếu không tìm thấy id.
    Device* findDevice(const std::string& id);

    bool removeDevice(const std::string& id);

    // Gọi simulate() cho TẤT CẢ thiết bị - đây là nơi tính ĐA HÌNH
    // (polymorphism) phát huy: ta KHÔNG cần biết từng device là loại gì,
    // chỉ cần gọi simulate(), C++ tự gọi đúng phiên bản override.
    void simulateAll();

    void printAllStatus() const;

    void turnOnDevice(const std::string& id);
    void turnOffDevice(const std::string& id);

    size_t getDeviceCount() const;

    // Tiện ích gọi FileStorage, giúp main.cpp không cần biết về FileStorage trực tiếp
    bool saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);
};

#endif // DEVICE_MANAGER_H