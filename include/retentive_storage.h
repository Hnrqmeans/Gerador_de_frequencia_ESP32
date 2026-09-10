#pragma once

#include <Arduino.h>

struct RetentiveSettings
{
    double frequencyHz;
    uint8_t dutyPercent;
};

class RetentiveStorage
{
public:
    bool begin();
    bool load(RetentiveSettings &settings) const;
    bool save(const RetentiveSettings &settings);

private:
    bool initialized_ = false;
};