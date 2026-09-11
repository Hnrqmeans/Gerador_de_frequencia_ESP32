/**
 * @file main.cpp
 * @author MoreirH
 * @brief Controle serial do gerador PWM para ESP32.
 * @version 1.0
 * @date 2026-09-11
 * @copyright Copyright (c) 2026 MoreirH
 */

#include <Arduino.h>

#include "config.h"
#include "pwm_generator.h"
#include "retentive_storage.h"

namespace
{
  PwmGenerator pwm;
  RetentiveStorage storage;

  void printStatus()
  {
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
      Serial.printf("Canal %u: %.3f Hz (real: %.3f Hz) | Duty: %u%%\n",
                    channel, pwm.frequency(channel),
                    channel == 0 ? pwm.actualFrequency() : pwm.frequency(channel),
                    pwm.duty(channel));
  }

  void saveSettings()
  {
    RetentiveSettings settings{};
    for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
    {
      settings.frequencyHz[channel] = pwm.frequency(channel);
      settings.dutyPercent[channel] = pwm.duty(channel);
    }
    storage.save(settings);
  }

  void printHelp()
  {
    Serial.println("Comandos:");
    Serial.println("  F<Hz>   frequencia em todos os canais (ex.: F1500)");
    Serial.println("  D<%>    duty em todos os canais (ex.: D50)");
    Serial.println("  F<n>:<Hz>  frequencia do canal n (ex.: F3:1500)");
    Serial.println("  D<n>:<%>   duty do canal n (ex.: D3:25)");
    Serial.println("  Tambem aceitos: F3=1500 ou F3 1500");
    Serial.println("  FA<Hz>/DA<%>  aplicar explicitamente a todos");
    Serial.println("  S       mostrar configuracao atual");
    Serial.println("  Modo: altere ConfiguredFrequencyMode em include/config.h");
    Serial.println("  TestProfile = distribuicao normal em torno de 12 Hz");
  }

  void handleCommand()
  {
    while (Serial.available() && isspace(Serial.peek()))
      Serial.read();

    if (!Serial.available())
      return;

    const char command = static_cast<char>(toupper(Serial.read()));
    String argument = Serial.readStringUntil('\n');
    argument.trim();

    if (command == 'S')
    {
      printStatus();
    }
    else if (command == 'F' || command == 'D')
    {
      bool allChannels = true;
      int channel = -1;
      String valueText = argument;
      int separator = argument.indexOf(':');
      if (separator < 0)
        separator = argument.indexOf('=');
      if (separator < 0)
        separator = argument.indexOf(' ');

      if (separator >= 0)
      {
        allChannels = false;
        channel = argument.substring(0, separator).toInt();
        valueText = argument.substring(separator + 1);
      }
      else if (argument.startsWith("A"))
        valueText = argument.substring(1);

      const double value = valueText.toFloat();
      const bool validChannel = channel >= 0 &&
                                channel < static_cast<int>(Config::ChannelCount);
      const bool validValue = command == 'F'
                                  ? value >= Config::MinFrequencyHz && value <= Config::MaxFrequencyHz
                                  : value >= 0 && value <= 100;

      if (validValue && (allChannels || validChannel))
      {
        if (command == 'F')
        {
          if (allChannels)
            pwm.setFrequency(value);
          else
            pwm.setFrequency(static_cast<uint8_t>(channel), value);
        }
        else
        {
          if (allChannels)
            pwm.setDuty(static_cast<uint8_t>(value));
          else
            pwm.setDuty(static_cast<uint8_t>(channel), static_cast<uint8_t>(value));
        }
        saveSettings();
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

  RetentiveSettings settings{};
  for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
  {
    settings.frequencyHz[channel] = Config::DefaultFrequencyHz;
    settings.dutyPercent[channel] = Config::DefaultDutyPercent;
  }
  storage.begin();
  storage.load(settings);

  for (uint8_t channel = 0; channel < Config::ChannelCount; ++channel)
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