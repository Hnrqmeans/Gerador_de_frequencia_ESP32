#include "calculation/pwm_calculations.h"

#include <math.h>

namespace PwmCalculations
{
    uint8_t selectPwmResolution(const double frequenciesHz[], size_t channelCount,
                                uint8_t maximumResolution, uint32_t clockFrequencyHz)
    {
        uint8_t resolution = maximumResolution;
        double highestFrequencyHz = 0.0;

        for (size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex)
            highestFrequencyHz = max(highestFrequencyHz, frequenciesHz[channelIndex]);

        while (resolution > 1 &&
               highestFrequencyHz * (1ULL << resolution) > clockFrequencyHz)
            --resolution;

        return resolution;
    }

    uint32_t roundHardwareFrequency(double frequencyHz)
    {
        return static_cast<uint32_t>(frequencyHz + 0.5);
    }

    uint32_t calculateMaximumDuty(uint8_t pwmResolution)
    {
        return (1UL << pwmResolution) - 1;
    }

    uint32_t calculateDutyValue(uint8_t dutyPercent, uint32_t maximumDuty)
    {
        return (static_cast<uint32_t>(dutyPercent) * maximumDuty) / 100;
    }

    uint32_t calculateSlowPwmDurationUs(double frequencyHz, uint8_t dutyPercent, uint8_t outputLevel)
    {
        const double pwmPeriodUs = 1000000.0 / frequencyHz;
        const double highTimeUs = pwmPeriodUs * dutyPercent / 100.0;
        const double outputDurationUs = outputLevel == HIGH
                                            ? highTimeUs
                                            : pwmPeriodUs - highTimeUs;
        return static_cast<uint32_t>(max(1.0, outputDurationUs));
    }

    double calculateNormalDistributionFrequency(double meanFrequencyHz, double standardDeviationHz,
                                                double uniformValueA, double uniformValueB)
    {
        const double normalValue = sqrt(-2.0 * log(uniformValueA)) *
                                   cos(2.0 * PI * uniformValueB);
        return max(0.0, meanFrequencyHz + normalValue * standardDeviationHz);
    }

    uint32_t calculateIndicatorPeriodMs(double frequencyHz)
    {
        return static_cast<uint32_t>(1000.0 / frequencyHz);
    }

    uint32_t calculateIndicatorHighTimeMs(uint32_t periodMs)
    {
        return periodMs / 2;
    }
}