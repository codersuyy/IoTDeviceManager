#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <atomic>

enum class DeviceStatus {
    ON,
    OFF
};

class Device {
protected:
    std::string id_;      
    std::string name_;    
    
    std::atomic<DeviceStatus> status_;
    std::atomic<int> batteryLevel_;

public:
    Device(const std::string& id, const std::string& name);

    virtual ~Device() = default;

    virtual void simulate() = 0;

    virtual std::string getReadingInfo() const = 0;

    virtual std::string getTypeName() const = 0;

    virtual std::string serializeExtra() const = 0;

    virtual void deserializeExtra(const std::string& data) = 0;

    void turnOn();
    void turnOff();
    bool isOn() const;

    std::string getId() const;
    std::string getName() const;
    int getBatteryLevel() const;
    DeviceStatus getStatus() const;

    void drainBattery(int amount);

    void setStatus(DeviceStatus status);
    void setBatteryLevel(int level);
};

#endif // DEVICE_H