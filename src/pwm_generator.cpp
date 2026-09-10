#include "pwm_generator.h"

#include "config.h"

void PwmGenerator::begin()
{
    apply();
}

void PwmGenerator::update()
{
    updateSlowPwm();
}

void PwmGenerator::setFrequency(double frequencyHz)
{
    frequencyHz_ = frequencyHz;
    apply();
}

void PwmGenerator::setDuty(uint8_t dutyPercent)
{
    dutyPercent_ = dutyPercent;
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
        ledcDetachPin(Config::LedPin);
        ledcAttached_ = false;
    }

    for (const uint8_t pin : Config::OutputPins)
        pinMode(pin, OUTPUT);
    pinMode(Config::LedPin, OUTPUT);

    actualFrequencyHz_ = frequencyHz_;
    slowCycleStartMs_ = millis();
    writeSlowOutput(LOW);
}

void PwmGenerator::applyHardwarePwm()
{
    pwmResolution_ = selectResolution();
    const uint32_t maxDuty = (1UL << pwmResolution_) - 1;
    const uint32_t duty = (static_cast<uint32_t>(dutyPercent_) * maxDuty) / 100;

    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        const double configuredFrequency = ledcSetup(
            channel, frequencyHz_, pwmResolution_);
        ledcAttachPin(Config::OutputPins[channel], channel);
        ledcWrite(channel, duty);

        if (channel == 0)
            actualFrequencyHz_ = configuredFrequency;
    }

    ledcAttachPin(Config::LedPin, 0);
    ledcAttached_ = true;
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
    digitalWrite(Config::LedPin, level);
}