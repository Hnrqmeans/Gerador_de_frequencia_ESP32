#include "pwm_generator.h"

#include "config.h"

void PwmGenerator::begin()
{
    if (Config::ConfiguredFrequencyMode == Config::FrequencyMode::TestProfile)
        initializeTestProfile();
    else
        for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
            currentFrequencyHz_[channel] = frequencyHz_[channel];

    initialized_ = true;
    apply();
}

void PwmGenerator::update()
{
    updateTestProfile();
    updateSlowPwm();
    updateLedIndicator();
}

void PwmGenerator::setFrequency(double frequencyHz)
{
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        frequencyHz_[channel] = frequencyHz;
        if (Config::ConfiguredFrequencyMode == Config::FrequencyMode::Configurable)
            currentFrequencyHz_[channel] = frequencyHz;
    }

    if (initialized_)
        apply();
}

void PwmGenerator::setFrequency(uint8_t channel, double frequencyHz)
{
    if (channel >= Config::ChannelCount)
        return;

    frequencyHz_[channel] = frequencyHz;
    if (Config::ConfiguredFrequencyMode == Config::FrequencyMode::Configurable)
        currentFrequencyHz_[channel] = frequencyHz;
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
    return channel < Config::ChannelCount ? currentFrequencyHz_[channel] : 0.0;
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
        highestFrequency = max(highestFrequency, currentFrequencyHz_[channel]);

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
        const bool useSoftwarePwm = Config::ConfiguredFrequencyMode == Config::FrequencyMode::TestProfile ||
                                    currentFrequencyHz_[channel] < 1.0;
        if (!useSoftwarePwm)
            continue;

        if (ledcAttached_[channel])
        {
            ledcDetachPin(Config::OutputPins[channel]);
            ledcAttached_[channel] = false;
        }

        pinMode(Config::OutputPins[channel], OUTPUT);
        slowMode_[channel] = true;
        actualFrequencyHz_[channel] = currentFrequencyHz_[channel];
        slowCycleStartMs_[channel] = millis();
        writeSlowOutput(channel, LOW);
        slowNextTransitionUs_[channel] = micros() +
                                         static_cast<uint32_t>(1000000.0 / currentFrequencyHz_[channel]);
    }

    pinMode(Config::LedPin, OUTPUT);
    resetLedIndicator();
}

void PwmGenerator::applyHardwarePwm()
{
    if (Config::ConfiguredFrequencyMode == Config::FrequencyMode::TestProfile)
        return;

    pwmResolution_ = selectResolution();
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        if (currentFrequencyHz_[channel] < 1.0)
            continue;

        const uint32_t hardwareFrequency = static_cast<uint32_t>(currentFrequencyHz_[channel] + 0.5);
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
    const uint32_t nowUs = micros();
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        if (!slowMode_[channel])
            continue;

        if (currentFrequencyHz_[channel] <= 0.0 || dutyPercent_[channel] == 0)
        {
            if (slowOutputState_[channel] != LOW)
                writeSlowOutput(channel, LOW);
            continue;
        }

        if (static_cast<int32_t>(nowUs - slowNextTransitionUs_[channel]) < 0)
            continue;

        const uint8_t level = slowOutputState_[channel] == LOW ? HIGH : LOW;
        writeSlowOutput(channel, level);

        const double periodUs = 1000000.0 / currentFrequencyHz_[channel];
        const double highTimeUs = periodUs * dutyPercent_[channel] / 100.0;
        const double durationUs = level == HIGH ? highTimeUs : periodUs - highTimeUs;
        slowNextTransitionUs_[channel] = nowUs + static_cast<uint32_t>(max(1.0, durationUs));
    }
}

void PwmGenerator::initializeTestProfile()
{
    randomSeed(micros());
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
        currentFrequencyHz_[channel] = Config::TestProfileMeanFrequencyHz;

    testProfileUpdateMs_ = millis();
}

void PwmGenerator::updateTestProfile()
{
    if (!initialized_ || Config::ConfiguredFrequencyMode != Config::FrequencyMode::TestProfile)
        return;

    const uint32_t now = millis();
    if (now - testProfileUpdateMs_ < Config::TestProfileUpdateIntervalMs)
        return;

    testProfileUpdateMs_ = now;
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        currentFrequencyHz_[channel] = testProfileFrequency();
        actualFrequencyHz_[channel] = currentFrequencyHz_[channel];
    }
}

double PwmGenerator::testProfileFrequency() const
{
    const double u1 = (static_cast<double>(random(1L, 1000000L))) / 1000000.0;
    const double u2 = (static_cast<double>(random(1L, 1000000L))) / 1000000.0;
    const double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * PI * u2);
    return max(0.0, Config::TestProfileMeanFrequencyHz +
                        z0 * Config::TestProfileSigmaHz);
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
                                 min(currentFrequencyHz_[channel], Config::LedIndicatorMaxFrequencyHz));
    const uint32_t periodMs = static_cast<uint32_t>(1000.0 / indicatorFrequency);
    const uint32_t highTimeMs = periodMs / 2;
    const uint32_t phaseMs = (millis() - ledCycleStartMs_) % periodMs;
    const uint8_t level = phaseMs < highTimeMs ? HIGH : LOW;

    if (level == ledOutputState_)
        return;

    ledOutputState_ = level;
    digitalWrite(Config::LedPin, level);
}