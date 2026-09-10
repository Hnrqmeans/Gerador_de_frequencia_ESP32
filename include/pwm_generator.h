#pragma once

#include <Arduino.h>

class PwmGenerator
{
public:
    void begin();
    void update();
    void setFrequency(double frequencyHz);
    void setDuty(uint8_t dutyPercent);

    double frequency() const;
    double actualFrequency() const;
    uint8_t duty() const;

private:
    void apply();
    void applyHardwarePwm();
    void applySlowPwm();
    void updateSlowPwm();
    void updateLedIndicator();
    void resetLedIndicator();
    uint8_t selectResolution() const;
    void writeSlowOutput(uint8_t level);

    double frequencyHz_ = 0.0;
    double actualFrequencyHz_ = 0.0;
    uint8_t dutyPercent_ = 0;
    uint8_t pwmResolution_ = 8;
    bool slowMode_ = false;
    bool ledcAttached_ = false;
    uint8_t slowOutputState_ = LOW;
    uint32_t slowCycleStartMs_ = 0;
    uint32_t ledCycleStartMs_ = 0;
    uint8_t ledOutputState_ = LOW;
    bool initialized_ = false;
};