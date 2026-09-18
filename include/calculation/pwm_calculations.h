#pragma once

#include <Arduino.h>

namespace PwmCalculations
{
    // Calcula a resolucao LEDC que suporta a maior frequencia configurada.
    uint8_t selectPwmResolution(const double frequenciesHz[], size_t channelCount,
                                uint8_t maximumResolution, uint32_t clockFrequencyHz);

    // Calculos de frequencia e duty cycle do PWM por hardware.
    uint32_t roundHardwareFrequency(double frequencyHz);
    uint32_t calculateMaximumDuty(uint8_t pwmResolution);
    uint32_t calculateDutyValue(uint8_t dutyPercent, uint32_t maximumDuty);

    // Calculos de tempo para o PWM por software e indicador visual.
    uint32_t calculateSlowPwmDurationUs(double frequencyHz, uint8_t dutyPercent, uint8_t outputLevel);
    uint32_t calculateIndicatorPeriodMs(double frequencyHz);
    uint32_t calculateIndicatorHighTimeMs(uint32_t periodMs);

    // Calculo estatistico usado pelo perfil de teste de frequencia.
    double calculateNormalDistributionFrequency(double meanFrequencyHz, double standardDeviationHz,
                                                double uniformValueA, double uniformValueB);
}