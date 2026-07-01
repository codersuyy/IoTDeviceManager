#ifndef SMART_LIGHT_H
#define SMART_LIGHT_H

#include "Device.h"

class SmartLight : public Device {
public:
    SmartLight(const std::string& id, const std::string& name);

    void simulate() override;
    std::string getReadingInfo() const override;

    std::string getTypeName() const override;
    std::string serializeExtra() const override;
    void deserializeExtra(const std::string& data) override;
};

#endif // SMART_LIGHT_H