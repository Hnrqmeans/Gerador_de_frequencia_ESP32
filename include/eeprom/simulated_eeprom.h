#pragma once

#include <Arduino.h>

#include "config/config.h"

// Configuracoes persistidas na NVS do ESP32, usada aqui como EEPROM simulada.
struct EepromSettings
{
    double frequencyHz[Config::channelCount];
    uint8_t dutyPercent[Config::channelCount];
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