#pragma once

#include <Arduino.h>

namespace Config
{
    enum class FrequencyMode : uint8_t
    {
        Configurable = 1,
        TestProfile = 2
    };

    constexpr double minFrequencyHz = 0.5;
    constexpr double maxFrequencyHz = 150000.0;
    constexpr double defaultFrequencyHz = 1.0;
    constexpr uint8_t defaultDutyPercent = 5;

    constexpr FrequencyMode defaultFrequencyMode = FrequencyMode::Configurable;
    constexpr double testProfileMeanFrequencyHz = 12.0;
    constexpr double testProfileSigmaHz = 3.0;
    constexpr uint32_t testProfileUpdateIntervalMs = 500;
    constexpr uint32_t minTestProfileUpdateIntervalMs = 50;

    constexpr uint8_t ledPin = 2;
    constexpr double ledIndicatorMaxFrequencyHz = 2.0;
    constexpr uint8_t maxPwmResolution = 20;
    constexpr uint32_t ledcClockHz = 80000000;

    const uint8_t outputPins[] = {
        4, 5, 13, 14, 16, 17, 18, 19,
        21, 22, 23, 25, 26, 27, 32, 33};
    constexpr size_t channelCount = sizeof(outputPins) / sizeof(outputPins[0]);
}