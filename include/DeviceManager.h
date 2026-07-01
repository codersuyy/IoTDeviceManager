#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "Device.h"
#include <vector>
#include <memory>   // std::unique_ptr
#include <string>
#include <map>

class DeviceManager {
private:
    std::vector<std::unique_ptr<Device>> devices_;
    std::map<std::string, bool> onlineStatus_;

public:
    DeviceManager() = default;

    // std::move: vì unique_ptr KHÔNG cho phép copy (chỉ 1 chủ sở hữu),
    // ta phải "chuyển quyền sở hữu" từ bên gọi vào trong vector_.
    void addDevice(std::unique_ptr<Device> device);

    Device* findDevice(const std::string& id);

    bool removeDevice(const std::string& id);

    void simulateAll();

    void printAllStatus() const;

    void turnOnDevice(const std::string& id);
    void turnOffDevice(const std::string& id);

    // Hàm MỚI - dùng riêng cho topic "/status" (kết nối mạng)
    void setOnlineStatus(const std::string& id, bool online);
    bool isOnline(const std::string& id) const;

    size_t getDeviceCount() const;

    bool saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);

    void updateOrCreateDevice(const std::string& typeName,
                               const std::string& id,
                               const std::string& name,
                               const std::string& statusStr,
                               int battery,
                               const std::string& extra);
};

#endif // DEVICE_MANAGER_H