#include "retentive_storage.h"

#include <Preferences.h>

namespace
{
    constexpr char StorageNamespace[] = "pwm_config";
    constexpr char LegacyFrequencyKey[] = "frequency";
    constexpr char LegacyDutyKey[] = "duty";
}

Preferences preferences;

bool RetentiveStorage::begin()
{
    initialized_ = preferences.begin(StorageNamespace, false);
    return initialized_;
}

bool RetentiveStorage::load(RetentiveSettings &settings) const
{
    if (!initialized_)
        return false;

    const bool hasLegacyFrequency = preferences.isKey(LegacyFrequencyKey);
    const bool hasLegacyDuty = preferences.isKey(LegacyDutyKey);
    const double legacyFrequency = hasLegacyFrequency
                                       ? preferences.getDouble(LegacyFrequencyKey, settings.frequencyHz[0])
                                       : settings.frequencyHz[0];
    const uint8_t legacyDuty = hasLegacyDuty
                                   ? preferences.getUChar(LegacyDutyKey, settings.dutyPercent[0])
                                   : settings.dutyPercent[0];

    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        char frequencyKey[16];
        char dutyKey[16];
        snprintf(frequencyKey, sizeof(frequencyKey), "frequency_%u", channel);
        snprintf(dutyKey, sizeof(dutyKey), "duty_%u", channel);
        if (preferences.isKey(frequencyKey))
            settings.frequencyHz[channel] = preferences.getDouble(
                frequencyKey, legacyFrequency);
        else if (hasLegacyFrequency)
            settings.frequencyHz[channel] = legacyFrequency;

        if (preferences.isKey(dutyKey))
            settings.dutyPercent[channel] = preferences.getUChar(
                dutyKey, legacyDuty);
        else if (hasLegacyDuty)
            settings.dutyPercent[channel] = legacyDuty;
    }
    return true;
}

bool RetentiveStorage::save(const RetentiveSettings &settings)
{
    if (!initialized_)
        return false;

    bool saved = true;
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
        char frequencyKey[16];
        char dutyKey[16];
        snprintf(frequencyKey, sizeof(frequencyKey), "frequency_%u", channel);
        snprintf(dutyKey, sizeof(dutyKey), "duty_%u", channel);
        saved = preferences.putDouble(frequencyKey, settings.frequencyHz[channel]) > 0 && saved;
        saved = preferences.putUChar(dutyKey, settings.dutyPercent[channel]) > 0 && saved;
    }
    return saved;
}