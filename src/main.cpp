/**
 * @file main.cpp
 * @author MoreirH
 * @brief Controle serial do gerador PWM para ESP32.
 * @version 1.0
 * @date 2026-09-11
 * @copyright Copyright (c) 2026 MoreirH
 */

#include <Arduino.h>

#include "config/config.h"
#include "pwm/pwm_generator.h"
#include "eeprom/simulated_eeprom.h"

namespace
{
  PwmGenerator pwm;
  SimulatedEeprom simulatedEeprom;
  bool settingsDirty = false;

  void printStatus()
  {
    Serial.printf("Modo: %s\n", pwm.frequencyMode() == Config::FrequencyMode::Configurable
                                    ? "Configurable"
                                    : "TestProfile");
    const TestProfileSettings profileSettings = pwm.testProfileSettings();
    Serial.printf("Perfil: media %.3f Hz | sigma %.3f Hz | intervalo %lu ms\n",
                  profileSettings.meanFrequencyHz, profileSettings.sigmaHz,
                  static_cast<unsigned long>(profileSettings.updateIntervalMs));
    Serial.printf("Alteracoes pendentes: %s\n", settingsDirty ? "SIM" : "NAO");
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
      // O primeiro canal informa a frequencia realmente aplicada pelo hardware.
      Serial.printf("Canal %u: %.3f Hz (real: %.3f Hz) | Duty: %u%%\n",
                    channel, pwm.frequency(channel),
                    channel == 0 ? pwm.actualFrequency() : pwm.frequency(channel),
                    pwm.duty(channel));
  }

  bool saveSettings()
  {
    EepromSettings settings{};
    for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
    {
      settings.frequencyHz[channel] = pwm.configuredFrequency(channel);
      settings.dutyPercent[channel] = pwm.duty(channel);
    }
    settings.frequencyMode = static_cast<uint8_t>(pwm.frequencyMode());
    const TestProfileSettings profileSettings = pwm.testProfileSettings();
    settings.testProfileMeanFrequencyHz = profileSettings.meanFrequencyHz;
    settings.testProfileSigmaHz = profileSettings.sigmaHz;
    settings.testProfileUpdateIntervalMs = profileSettings.updateIntervalMs;
    const bool savedSuccessfully = simulatedEeprom.saveSettings(settings);
    if (savedSuccessfully)
      settingsDirty = false;
    return savedSuccessfully;
  }

  void printHelp()
  {
    Serial.println("Comandos:");
    Serial.println("  F<Hz>   frequencia em todos os canais (ex.: F1500)");
    Serial.println("  D<%>    duty em todos os canais (ex.: D50)");
    Serial.println("  F<n>:<Hz>  frequencia do canal n (ex.: F3:1500)");
    Serial.println("  D<n>:<%>   duty do canal n (ex.: D3:25)");
    Serial.println("  M1       modo configuravel");
    Serial.println("  M2       modo perfil de teste");
    Serial.println("  P<media>:<sigma>:<intervalo> (ex.: P12:3:500)");
    Serial.println("  S       mostrar configuracao atual");
    Serial.println("  SAVE    salvar alteracoes na EEPROM simulada");
    Serial.println("  F no TestProfile atualiza a frequencia manual pendente para o proximo M1");
  }

  void handleCommand()
  {
    while (Serial.available() && isspace(Serial.peek()))
      Serial.read();

    if (!Serial.available())
      return;

    String commandLine = Serial.readStringUntil('\n');
    commandLine.trim();
    commandLine.toUpperCase();
    if (commandLine.isEmpty())
      return;

    // SAVE precisa ser a linha inteira para evitar gravacoes acidentais.
    if (commandLine == "SAVE")
    {
      if (!settingsDirty)
      {
        Serial.println("Nenhuma alteracao pendente para salvar.");
      }
      else if (saveSettings())
      {
        Serial.println("Alteracoes salvas na EEPROM simulada.");
      }
      else
      {
        Serial.println("Falha ao salvar na EEPROM simulada.");
      }
      printStatus();
      return;
    }

    // O primeiro caractere define a operacao; o restante contem o argumento.
    const char command = commandLine.charAt(0);
    String argument = commandLine.substring(1);
    argument.trim();

    if (command == 'S')
    {
      printStatus();
    }
    else if (command == 'M')
    {
      const int modeValue = argument.toInt();
      if (modeValue == 1 || modeValue == 2)
      {
        pwm.setFrequencyMode(modeValue == 1
                                 ? Config::FrequencyMode::Configurable
                                 : Config::FrequencyMode::TestProfile);
        settingsDirty = true;
        printStatus();
      }
      else
      {
        Serial.println("Modo invalido. Use M1 ou M2.");
      }
    }
    else if (command == 'P')
    {
      const int firstSeparator = argument.indexOf(':');
      const int secondSeparator = argument.indexOf(':', firstSeparator + 1);
      if (firstSeparator > 0 && secondSeparator > firstSeparator)
      {
        TestProfileSettings profileSettings = pwm.testProfileSettings();
        profileSettings.meanFrequencyHz = argument.substring(0, firstSeparator).toFloat();
        profileSettings.sigmaHz = argument.substring(
                                              firstSeparator + 1, secondSeparator)
                                      .toFloat();
        profileSettings.updateIntervalMs = static_cast<uint32_t>(argument.substring(
                                                                             secondSeparator + 1)
                                                                     .toInt());
        const bool validProfile = profileSettings.meanFrequencyHz >= Config::minFrequencyHz &&
                                  profileSettings.meanFrequencyHz <= Config::maxFrequencyHz &&
                                  profileSettings.sigmaHz >= 0.0 &&
                                  profileSettings.updateIntervalMs >= Config::minTestProfileUpdateIntervalMs;
        if (validProfile)
        {
          pwm.setTestProfileSettings(profileSettings);
          settingsDirty = true;
          printStatus();
        }
        else
        {
          Serial.println("Perfil invalido. Verifique media, sigma e intervalo.");
        }
      }
      else
      {
        Serial.println("Perfil invalido. Use P<media>:<sigma>:<intervalo>.");
      }
    }
    else if (command == 'F' || command == 'D')
    {
      bool allChannels = true;
      int channelIndex = -1;
      String valueText = argument;
      int separator = argument.indexOf(':');
      if (separator < 0)
        separator = argument.indexOf('=');
      if (separator < 0)
        separator = argument.indexOf(' ');

      if (separator >= 0)
      {
        allChannels = false;
        channelIndex = argument.substring(0, separator).toInt();
        valueText = argument.substring(separator + 1);
      }
      const double value = valueText.toFloat();
      const bool validChannel = channelIndex >= 0 &&
                                channelIndex < static_cast<int>(Config::channelCount);
      const bool validValue = command == 'F'
                                  ? value >= Config::minFrequencyHz && value <= Config::maxFrequencyHz
                                  : value >= 0 && value <= 100;

      if (validValue && (allChannels || validChannel))
      {
        if (command == 'F')
        {
          if (allChannels)
            pwm.setFrequency(value);
          else
            pwm.setFrequency(static_cast<uint8_t>(channelIndex), value);
        }
        else
        {
          if (allChannels)
            pwm.setDuty(static_cast<uint8_t>(value));
          else
            pwm.setDuty(static_cast<uint8_t>(channelIndex), static_cast<uint8_t>(value));
        }
        settingsDirty = true;
        printStatus();
      }
      else
      {
        Serial.println("Comando ou valor invalido.");
        printHelp();
      }
    }
    else
    {
      Serial.println("Comando ou valor invalido.");
      printHelp();
    }

    while (Serial.available())
      Serial.read();
  }
}

void setup()
{
  Serial.begin(115200);

  EepromSettings settings{};
  for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
  {
    settings.frequencyHz[channel] = Config::defaultFrequencyHz;
    settings.dutyPercent[channel] = Config::defaultDutyPercent;
  }
  simulatedEeprom.begin();
  simulatedEeprom.loadSettings(settings);

  pwm.setTestProfileSettings({settings.testProfileMeanFrequencyHz,
                              settings.testProfileSigmaHz,
                              settings.testProfileUpdateIntervalMs});
  pwm.setFrequencyMode(settings.frequencyMode == static_cast<uint8_t>(Config::FrequencyMode::TestProfile)
                           ? Config::FrequencyMode::TestProfile
                           : Config::FrequencyMode::Configurable);

  for (uint8_t channel = 0; channel < Config::channelCount; ++channel)
  {
    pwm.setFrequency(channel, settings.frequencyHz[channel]);
    pwm.setDuty(channel, settings.dutyPercent[channel]);
  }
  pwm.begin();
  Serial.println("Gerador PWM ESP32 pronto: 16 canais, nivel de 3,3 V.");
  printStatus();
  printHelp();
}

void loop()
{
  pwm.update();
  handleCommand();
}