#pragma once

#include <Arduino.h>

#include "config/config.h"

// Configuracoes persistidas na NVS do ESP32, usada aqui como EEPROM simulada.
struct EepromSettings
{
    uint8_t frequencyMode = static_cast<uint8_t>(Config::defaultFrequencyMode);
    double frequencyHz[Config::channelCount];
    uint8_t dutyPercent[Config::channelCount];
    double testProfileMeanFrequencyHz = Config::testProfileMeanFrequencyHz;
    double testProfileSigmaHz = Config::testProfileSigmaHz;
    uint32_t testProfileUpdateIntervalMs = Config::testProfileUpdateIntervalMs;
};

class SimulatedEeprom
{
public:
    // Abre a NVS que representa a EEPROM simulada nesta aplicacao.
    bool begin();
    // Carrega configuracoes persistidas sem alterar os valores padrao recebidos.
    bool loadSettings(EepromSettings &settings) const;
    // Persiste configuracoes por canal na NVS usada como EEPROM simulada.
    bool saveSettings(const EepromSettings &settings);

private:
    bool initialized_ = false;
};