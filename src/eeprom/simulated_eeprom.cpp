#include "eeprom/simulated_eeprom.h"

#include <Preferences.h>

namespace
{
    // Preferences grava na NVS; este modulo fornece a interface de EEPROM simulada.
    constexpr char eepromNamespace[] = "pwm_config";
    constexpr char legacyFrequencyKey[] = "frequency";
    constexpr char legacyDutyKey[] = "duty";
}

Preferences simulatedEepromStorage;

bool SimulatedEeprom::begin()
{
    initialized_ = simulatedEepromStorage.begin(eepromNamespace, false);
    return initialized_;
}

bool SimulatedEeprom::loadSettings(EepromSettings &settings) const
{
    if (!initialized_)
        return false;

    const bool hasLegacyFrequency = simulatedEepromStorage.isKey(legacyFrequencyKey);
    const bool hasLegacyDuty = simulatedEepromStorage.isKey(legacyDutyKey);
    const double legacyFrequency = hasLegacyFrequency
                                       ? simulatedEepromStorage.getDouble(legacyFrequencyKey, settings.frequencyHz[0])
                                       : settings.frequencyHz[0];
    const uint8_t legacyDuty = hasLegacyDuty
                                   ? simulatedEepromStorage.getUChar(legacyDutyKey, settings.dutyPercent[0])
                                   : settings.dutyPercent[0];

    for (uint8_t channelIndex = 0; channelIndex < Config::channelCount; ++channelIndex)
    {
        char frequencyKey[16];
        char dutyKey[16];
        snprintf(frequencyKey, sizeof(frequencyKey), "frequency_%u", channelIndex);
        snprintf(dutyKey, sizeof(dutyKey), "duty_%u", channelIndex);

        if (simulatedEepromStorage.isKey(frequencyKey))
            settings.frequencyHz[channelIndex] = simulatedEepromStorage.getDouble(frequencyKey, legacyFrequency);
        else if (hasLegacyFrequency)
            settings.frequencyHz[channelIndex] = legacyFrequency;

        if (simulatedEepromStorage.isKey(dutyKey))
            settings.dutyPercent[channelIndex] = simulatedEepromStorage.getUChar(dutyKey, legacyDuty);
        else if (hasLegacyDuty)
            settings.dutyPercent[channelIndex] = legacyDuty;
    }

    return true;
}

bool SimulatedEeprom::saveSettings(const EepromSettings &settings)
{
    if (!initialized_)
        return false;

    bool savedSuccessfully = true;
    for (uint8_t channelIndex = 0; channelIndex < Config::channelCount; ++channelIndex)
    {
        char frequencyKey[16];
        char dutyKey[16];
        snprintf(frequencyKey, sizeof(frequencyKey), "frequency_%u", channelIndex);
        snprintf(dutyKey, sizeof(dutyKey), "duty_%u", channelIndex);
        savedSuccessfully = simulatedEepromStorage.putDouble(
                                frequencyKey, settings.frequencyHz[channelIndex]) > 0 &&
                            savedSuccessfully;
        savedSuccessfully = simulatedEepromStorage.putUChar(
                                dutyKey, settings.dutyPercent[channelIndex]) > 0 &&
                            savedSuccessfully;
    }

    return savedSuccessfully;
}