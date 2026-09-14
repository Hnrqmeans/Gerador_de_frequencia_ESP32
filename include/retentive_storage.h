#pragma once

#include <Arduino.h>

#include "config.h"

struct RetentiveSettings
{
    double frequencyHz[Config::ChannelCount];
    uint8_t dutyPercent[Config::ChannelCount];
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