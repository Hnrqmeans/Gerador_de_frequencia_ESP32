#pragma once

#include <Arduino.h>

class PwmGenerator
{
public:
    void begin();
    void update();
    void setFrequency(double frequencyHz);
    void setFrequency(uint8_t channel, double frequencyHz);
    void setDuty(uint8_t dutyPercent);
    void setDuty(uint8_t channel, uint8_t dutyPercent);

    double frequency() const;
    double frequency(uint8_t channel) const;
    double actualFrequency() const;
    uint8_t duty() const;
    uint8_t duty(uint8_t channel) const;

private:
    void apply();
    void applyHardwarePwm();
    void applySlowPwm();
    void updateSlowPwm();
    void updateLedIndicator();
    void resetLedIndicator();
    uint8_t selectResolution() const;
    void writeSlowOutput(uint8_t channel, uint8_t level);

    double frequencyHz_[16] = {};
    double actualFrequencyHz_[16] = {};
    uint8_t dutyPercent_[16] = {};
    uint8_t pwmResolution_ = 8;
    bool slowMode_[16] = {};
    bool ledcAttached_[16] = {};
    uint8_t slowOutputState_[16] = {};
    uint32_t slowCycleStartMs_[16] = {};
    uint32_t ledCycleStartMs_ = 0;
    uint8_t ledOutputState_ = LOW;
    bool initialized_ = false;
};