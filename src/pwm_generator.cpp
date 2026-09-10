#include "pwm_generator.h"

#include "config.h"

void PwmGenerator::begin()
{
    initialized_ = true;
    apply();
}

void PwmGenerator::update()
{
    updateSlowPwm();
    updateLedIndicator();
}

void PwmGenerator::setFrequency(double frequencyHz)
{
    frequencyHz_ = frequencyHz;
    if (initialized_)
        apply();
}

void PwmGenerator::setDuty(uint8_t dutyPercent)
{
    dutyPercent_ = dutyPercent;
    if (initialized_)
        apply();
}

double PwmGenerator::frequency() const
{
    return frequencyHz_;
}

double PwmGenerator::actualFrequency() const
{
    return actualFrequencyHz_;
}

uint8_t PwmGenerator::duty() const
{
    return dutyPercent_;
}

uint8_t PwmGenerator::selectResolution() const
{
    uint8_t resolution = Config::MaxPwmResolution;

    while (resolution > 1 &&
           frequencyHz_ * (1ULL << resolution) > Config::LedcClockHz)
        --resolution;

    return resolution;
}

void PwmGenerator::apply()
{
    slowMode_ = frequencyHz_ < 1.0;

    if (slowMode_)
    {
        applySlowPwm();
        return;
    }

    applyHardwarePwm();
}

void PwmGenerator::applySlowPwm()
{
    if (ledcAttached_)
    {
        for (const uint8_t pin : Config::OutputPins)
            ledcDetachPin(pin);
        ledcAttached_ = false;
    }

    for (const uint8_t pin : Config::OutputPins)
        pinMode(pin, OUTPUT);
    pinMode(Config::LedPin, OUTPUT);

    actualFrequencyHz_ = frequencyHz_;
    slowCycleStartMs_ = millis();
    writeSlowOutput(LOW);
    resetLedIndicator();
}

void PwmGenerator::applyHardwarePwm()
{
    pwmResolution_ = selectResolution();
    const uint32_t hardwareFrequency = static_cast<uint32_t>(frequencyHz_ + 0.5);
    const uint32_t maxDuty = (1UL << pwmResolution_) - 1;
    const uint32_t duty = (static_cast<uint32_t>(dutyPercent_) * maxDuty) / 100;

    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        ledcSetup(channel, hardwareFrequency, pwmResolution_);
        ledcAttachPin(Config::OutputPins[channel], channel);
        ledcWrite(channel, duty);

        if (channel == 0)
            actualFrequencyHz_ = hardwareFrequency;
    }

    ledcAttached_ = true;
    pinMode(Config::LedPin, OUTPUT);
    resetLedIndicator();
}

void PwmGenerator::updateSlowPwm()
{
    if (!slowMode_)
        return;

    const uint32_t periodMs = static_cast<uint32_t>(1000.0 / frequencyHz_);
    const uint32_t highTimeMs = (periodMs * dutyPercent_) / 100;
    const uint32_t elapsedMs = millis() - slowCycleStartMs_;
    const uint32_t phaseMs = elapsedMs % periodMs;
    const uint8_t level = phaseMs < highTimeMs ? HIGH : LOW;

    if (level != slowOutputState_)
        writeSlowOutput(level);
}

void PwmGenerator::writeSlowOutput(uint8_t level)
{
    slowOutputState_ = level;

    for (const uint8_t pin : Config::OutputPins)
        digitalWrite(pin, level);
}

void PwmGenerator::resetLedIndicator()
{
    ledCycleStartMs_ = millis();
    ledOutputState_ = LOW;
    digitalWrite(Config::LedPin, LOW);
}

void PwmGenerator::updateLedIndicator()
{
    const double indicatorFrequency = min(
        frequencyHz_, Config::LedIndicatorMaxFrequencyHz);
    const uint32_t periodMs = static_cast<uint32_t>(1000.0 / indicatorFrequency);
    const uint32_t highTimeMs = periodMs / 2;
    const uint32_t phaseMs = (millis() - ledCycleStartMs_) % periodMs;
    const uint8_t level = phaseMs < highTimeMs ? HIGH : LOW;

    if (level == ledOutputState_)
        return;

    ledOutputState_ = level;
    digitalWrite(Config::LedPin, level);
}