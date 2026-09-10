#include "retentive_storage.h"

#include <Preferences.h>

namespace
{
    constexpr char StorageNamespace[] = "pwm_config";
    constexpr char FrequencyKey[] = "frequency";
    constexpr char DutyKey[] = "duty";
}

Preferences preferences;

bool RetentiveStorage::begin()
{
    initialized_ = preferences.begin(StorageNamespace, false);
    return initialized_;
}

bool RetentiveStorage::load(RetentiveSettings &settings) const
{
    if (!initialized_ || !preferences.isKey(FrequencyKey) ||
        !preferences.isKey(DutyKey))
        return false;

    settings.frequencyHz = preferences.getDouble(FrequencyKey, settings.frequencyHz);
    settings.dutyPercent = preferences.getUChar(DutyKey, settings.dutyPercent);
    return true;
}

bool RetentiveStorage::save(const RetentiveSettings &settings)
{
    if (!initialized_)
        return false;

    const size_t frequencyBytes = preferences.putDouble(FrequencyKey, settings.frequencyHz);
    const size_t dutyBytes = preferences.putUChar(DutyKey, settings.dutyPercent);
    return frequencyBytes > 0 && dutyBytes > 0;
}