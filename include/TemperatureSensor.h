#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include "Device.h"

class TemperatureSensor : public Device {
private:
    double currentTemp_;  
    double minTemp_;
    double maxTemp_;

public:
    TemperatureSensor(const std::string& id, const std::string& name,
                       double minTemp = 18.0, double maxTemp = 35.0);

    void simulate() override;
    std::string getReadingInfo() const override;

    std::string getTypeName() const override;
    std::string serializeExtra() const override;
    void deserializeExtra(const std::string& data) override;

    double getCurrentTemp() const;
};

#endif // TEMPERATURE_SENSOR_H