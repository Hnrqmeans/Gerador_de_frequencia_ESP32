
#include <Arduino.h>

#include "config.h"
#include "pwm_generator.h"

namespace
{
  PwmGenerator pwm;

  void printStatus()
  {
    Serial.printf("Frequencia: %.3f Hz (real: %.3f Hz) | Duty: %u%% | Canais: %u\n",
                  pwm.frequency(), pwm.actualFrequency(), pwm.duty(),
                  static_cast<unsigned>(Config::ChannelCount));
  }

  void printHelp()
  {
    Serial.println("Comandos:");
    Serial.println("  F<Hz>   frequencia de 0.5 a 150000 Hz (ex.: F150000 ou F0.5)");
    Serial.println("  D<%>    duty cycle de 0 a 100 (ex.: D50)");
    Serial.println("  S       mostrar configuracao atual");
  }

  void handleCommand()
  {
    if (!Serial.available())
      return;

    const char command = static_cast<char>(toupper(Serial.read()));
    const double value = Serial.parseFloat();

    if (command == 'F' && value >= Config::MinFrequencyHz &&
        value <= Config::MaxFrequencyHz)
    {
      pwm.setFrequency(value);
      printStatus();
    }
    else if (command == 'D' && value >= 0 && value <= 100)
    {
      pwm.setDuty(static_cast<uint8_t>(value));
      printStatus();
    }
    else if (command == 'S')
    {
      printStatus();
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

  pwm.setFrequency(Config::DefaultFrequencyHz);
  pwm.setDuty(Config::DefaultDutyPercent);
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