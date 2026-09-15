#include "pwm/pwm_generator.h"

#include "calculation/pwm_calculations.h"
#include "config/config.h"

void PwmGenerator::begin()
{
    if (Config::configuredFrequencyMode == Config::FrequencyMode::TestProfile)
        initializeTestProfile();
    else
        for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
            currentFrequencyHz_[channel] = configuredFrequencyHz_[channel];

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
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
    {
        configuredFrequencyHz_[channel] = frequencyHz;
        if (Config::configuredFrequencyMode == Config::FrequencyMode::Configurable)
            currentFrequencyHz_[channel] = frequencyHz;
    }

    if (initialized_)
        apply();
}

void PwmGenerator::setFrequency(uint8_t channel, double frequencyHz)
{
    if (channel >= Config::channelCount)
        return;

    configuredFrequencyHz_[channel] = frequencyHz;
    if (Config::configuredFrequencyMode == Config::FrequencyMode::Configurable)
        currentFrequencyHz_[channel] = frequencyHz;
    if (initialized_)
        apply();
}

void PwmGenerator::setDuty(uint8_t dutyPercent)
{
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
        dutyPercent_[channel] = dutyPercent;

    if (initialized_)
        apply();
}

void PwmGenerator::setDuty(uint8_t channel, uint8_t dutyPercent)
{
    if (channel >= Config::channelCount)
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
    return channel < Config::channelCount ? currentFrequencyHz_[channel] : 0.0;
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
    return channel < Config::channelCount ? dutyPercent_[channel] : 0;
}

void PwmGenerator::apply()
{
    applySlowPwm();
    applyHardwarePwm();
}

void PwmGenerator::applySlowPwm()
{
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
    {
        const bool useSoftwarePwm = Config::configuredFrequencyMode == Config::FrequencyMode::TestProfile ||
                                    currentFrequencyHz_[channel] < 1.0;
        if (!useSoftwarePwm)
            continue;

        if (ledcAttached_[channel])
        {
            ledcDetachPin(Config::outputPins[channel]);
            ledcAttached_[channel] = false;
        }

        pinMode(Config::outputPins[channel], OUTPUT);
        slowMode_[channel] = true;
        actualFrequencyHz_[channel] = currentFrequencyHz_[channel];
        slowCycleStartMs_[channel] = millis();
        writeSlowOutput(channel, LOW);
        slowNextTransitionUs_[channel] = micros() +
                                         static_cast<uint32_t>(1000000.0 / currentFrequencyHz_[channel]);
    }

    pinMode(Config::ledPin, OUTPUT);
    resetLedIndicator();
}

void PwmGenerator::applyHardwarePwm()
{
    if (Config::configuredFrequencyMode == Config::FrequencyMode::TestProfile)
        return;

    pwmResolution_ = PwmCalculations::selectPwmResolution(
        currentFrequencyHz_, Config::channelCount,
        Config::maxPwmResolution, Config::ledcClockHz);
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
    {
        if (currentFrequencyHz_[channel] < 1.0)
            continue;

        const uint32_t hardwareFrequency = PwmCalculations::roundHardwareFrequency(
            currentFrequencyHz_[channel]);
        const uint32_t maximumDuty = PwmCalculations::calculateMaximumDuty(pwmResolution_);
        const uint32_t dutyValue = PwmCalculations::calculateDutyValue(
            dutyPercent_[channel], maximumDuty);

        slowMode_[channel] = false;
        ledcSetup(channel, hardwareFrequency, pwmResolution_);
        ledcAttachPin(Config::outputPins[channel], channel);
        ledcWrite(channel, dutyValue);
        actualFrequencyHz_[channel] = hardwareFrequency;
        ledcAttached_[channel] = true;
    }

    pinMode(Config::ledPin, OUTPUT);
    resetLedIndicator();
}

void PwmGenerator::updateSlowPwm()
{
    const uint32_t nowUs = micros();
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
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

        const uint32_t durationUs = PwmCalculations::calculateSlowPwmDurationUs(
            currentFrequencyHz_[channel], dutyPercent_[channel], level);
        slowNextTransitionUs_[channel] = nowUs + durationUs;
    }
}

void PwmGenerator::initializeTestProfile()
{
    randomSeed(micros());
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
        currentFrequencyHz_[channel] = Config::testProfileMeanFrequencyHz;

    testProfileUpdateMs_ = millis();
}

void PwmGenerator::updateTestProfile()
{
    if (!initialized_ || Config::configuredFrequencyMode != Config::FrequencyMode::TestProfile)
        return;

    const uint32_t nowMs = millis();
    if (nowMs - testProfileUpdateMs_ < Config::testProfileUpdateIntervalMs)
        return;

    testProfileUpdateMs_ = nowMs;
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
    {
        const double uniformValueA = static_cast<double>(random(1L, 1000000L)) / 1000000.0;
        const double uniformValueB = static_cast<double>(random(1L, 1000000L)) / 1000000.0;
        currentFrequencyHz_[channel] = PwmCalculations::calculateNormalDistributionFrequency(
            Config::testProfileMeanFrequencyHz, Config::testProfileSigmaHz,
            uniformValueA, uniformValueB);
        actualFrequencyHz_[channel] = currentFrequencyHz_[channel];
    }
}

void PwmGenerator::writeSlowOutput(uint8_t channel, uint8_t level)
{
    slowOutputState_[channel] = level;
    digitalWrite(Config::outputPins[channel], level);
}

void PwmGenerator::resetLedIndicator()
{
    ledCycleStartMs_ = millis();
    ledOutputState_ = LOW;
    digitalWrite(Config::ledPin, LOW);
}

void PwmGenerator::updateLedIndicator()
{
    double indicatorFrequency = Config::minFrequencyHz;
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
        indicatorFrequency = max(indicatorFrequency,
                                 min(currentFrequencyHz_[channel], Config::ledIndicatorMaxFrequencyHz));
    const uint32_t periodMs = PwmCalculations::calculateIndicatorPeriodMs(indicatorFrequency);
    const uint32_t highTimeMs = PwmCalculations::calculateIndicatorHighTimeMs(periodMs);
    const uint32_t phaseMs = (millis() - ledCycleStartMs_) % periodMs;
    const uint8_t level = phaseMs < highTimeMs ? HIGH : LOW;

    if (level == ledOutputState_)
        return;

    ledOutputState_ = level;
    digitalWrite(Config::ledPin, level);
}