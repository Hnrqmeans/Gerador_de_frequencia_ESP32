#pragma once

#include <Arduino.h>

namespace Config
{
    constexpr double MinFrequencyHz = 0.5; // 0.5 1s~
    constexpr double MaxFrequencyHz = 150000.0;
    constexpr double DefaultFrequencyHz = 1; // Hz
    constexpr uint8_t DefaultDutyPercent = 50;

    constexpr uint8_t LedPin = 2;
    constexpr uint8_t MaxPwmResolution = 20;
    constexpr uint32_t LedcClockHz = 80000000;

    const uint8_t OutputPins[] = {
        4, 5, 13, 14, 16, 17, 18, 19,
        21, 22, 23, 25, 26, 27, 32, 33};
    constexpr size_t ChannelCount = sizeof(OutputPins) / sizeof(OutputPins[0]);
}