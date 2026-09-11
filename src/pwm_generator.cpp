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
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
        frequencyHz_[channel] = frequencyHz;

    if (initialized_)
        apply();
}

void PwmGenerator::setFrequency(uint8_t channel, double frequencyHz)
{
    if (channel >= Config::ChannelCount)
        return;

    frequencyHz_[channel] = frequencyHz;
    if (initialized_)
        apply();
}

void PwmGenerator::setDuty(uint8_t dutyPercent)
{
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
        dutyPercent_[channel] = dutyPercent;

    if (initialized_)
        apply();
}

void PwmGenerator::setDuty(uint8_t channel, uint8_t dutyPercent)
{
    if (channel >= Config::ChannelCount)
        return;

    dutyPercent_[channel] = dutyPercent;
    if (initialized_)
        apply();
}

double PwmGenerator::frequency() const
{
    return frequency(0);
}

double PwmGenerator::frequency(uint8_t channel) const
{
    return channel < Config::ChannelCount ? frequencyHz_[channel] : 0.0;
}

double PwmGenerator::actualFrequency() const
{
    return actualFrequencyHz_[0];
}

uint8_t PwmGenerator::duty() const
{
    return duty(0);
}

uint8_t PwmGenerator::duty(uint8_t channel) const
{
    return channel < Config::ChannelCount ? dutyPercent_[channel] : 0;
}

uint8_t PwmGenerator::selectResolution() const
{
    uint8_t resolution = Config::MaxPwmResolution;
    double highestFrequency = 0.0;

    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
        highestFrequency = max(highestFrequency, frequencyHz_[channel]);

    while (resolution > 1 &&
           highestFrequency * (1ULL << resolution) > Config::LedcClockHz)
        --resolution;

    return resolution;
}

void PwmGenerator::apply()
{
    applySlowPwm();
    applyHardwarePwm();
}

void PwmGenerator::applySlowPwm()
{
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        if (frequencyHz_[channel] >= 1.0)
            continue;

        if (ledcAttached_[channel])
        {
            ledcDetachPin(Config::OutputPins[channel]);
            ledcAttached_[channel] = false;
        }

        pinMode(Config::OutputPins[channel], OUTPUT);
        slowMode_[channel] = true;
        actualFrequencyHz_[channel] = frequencyHz_[channel];
        slowCycleStartMs_[channel] = millis();
        writeSlowOutput(channel, LOW);
    }

    pinMode(Config::LedPin, OUTPUT);
    resetLedIndicator();
}

void PwmGenerator::applyHardwarePwm()
{
    pwmResolution_ = selectResolution();
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        if (frequencyHz_[channel] < 1.0)
            continue;

        const uint32_t hardwareFrequency = static_cast<uint32_t>(frequencyHz_[channel] + 0.5);
        const uint32_t maxDuty = (1UL << pwmResolution_) - 1;
        const uint32_t duty = (static_cast<uint32_t>(dutyPercent_[channel]) * maxDuty) / 100;

        slowMode_[channel] = false;
        ledcSetup(channel, hardwareFrequency, pwmResolution_);
        ledcAttachPin(Config::OutputPins[channel], channel);
        ledcWrite(channel, duty);
        actualFrequencyHz_[channel] = hardwareFrequency;
        ledcAttached_[channel] = true;
    }

    pinMode(Config::LedPin, OUTPUT);
    resetLedIndicator();
}

void PwmGenerator::updateSlowPwm()
{
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        if (!slowMode_[channel])
            continue;

        const uint32_t periodMs = static_cast<uint32_t>(1000.0 / frequencyHz_[channel]);
        const uint32_t highTimeMs = (periodMs * dutyPercent_[channel]) / 100;
        const uint32_t elapsedMs = millis() - slowCycleStartMs_[channel];
        const uint32_t phaseMs = elapsedMs % periodMs;
        const uint8_t level = phaseMs < highTimeMs ? HIGH : LOW;

        if (level != slowOutputState_[channel])
            writeSlowOutput(channel, level);
    }
}

void PwmGenerator::writeSlowOutput(uint8_t channel, uint8_t level)
{
    slowOutputState_[channel] = level;
    digitalWrite(Config::OutputPins[channel], level);
}

void PwmGenerator::resetLedIndicator()
{
    ledCycleStartMs_ = millis();
    ledOutputState_ = LOW;
    digitalWrite(Config::LedPin, LOW);
}

void PwmGenerator::updateLedIndicator()
{
    double indicatorFrequency = Config::MinFrequencyHz;
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
        indicatorFrequency = max(indicatorFrequency,
                                 min(frequencyHz_[channel], Config::LedIndicatorMaxFrequencyHz));
    const uint32_t periodMs = static_cast<uint32_t>(1000.0 / indicatorFrequency);
    const uint32_t highTimeMs = periodMs / 2;
    const uint32_t phaseMs = (millis() - ledCycleStartMs_) % periodMs;
    const uint8_t level = phaseMs < highTimeMs ? HIGH : LOW;

    if (level == ledOutputState_)
        return;

    ledOutputState_ = level;
    digitalWrite(Config::LedPin, level);
}