
/*
GPIO 4  -> canal de monitoramento 1
GPIO 5  -> canal de monitoramento 2
GPIO 13 -> canal de monitoramento 3
...
GND ESP32 -> GND do sistema monitorado

Comandos especificos
Min -> F1       -> 1 Hz
Max= F150000    -> 150 kHz
D50        -> duty cycle de 50%
D10        -> duty cycle de 10%
*/

#include <Arduino.h>

namespace
{
  constexpr uint32_t MinFrequency = 1;
  constexpr uint32_t MaxFrequency = 150000;
  constexpr uint8_t PwmResolution = 8;
  constexpr uint16_t MaxDuty = (1 << PwmResolution) - 1;

  const uint8_t OutputPins[] = {
      4, 5, 13, 14, 16, 17, 18, 19,
      21, 22, 23, 25, 26, 27, 32, 33};
  constexpr size_t ChannelCount = sizeof(OutputPins) / sizeof(OutputPins[0]);

  uint32_t frequency = 10;
  uint8_t dutyPercent = 50;

  void applyPwm()
  {
    const uint32_t duty = (static_cast<uint32_t>(dutyPercent) * MaxDuty) / 100;

    for (uint8_t channel = 0; channel < ChannelCount; ++channel)
    {
      ledcSetup(channel, frequency, PwmResolution);
      ledcWrite(channel, duty);
    }
  }

  void printStatus()
  {
    Serial.printf("Frequencia: %lu Hz | Duty: %u%% | Canais: %u\n",
                  static_cast<unsigned long>(frequency), dutyPercent,
                  static_cast<unsigned>(ChannelCount));
  }

  void printHelp()
  {
    Serial.println("Comandos:");
    Serial.println("  F<Hz>   frequencia de 1 a 150000 Hz (ex.: F150000)");
    Serial.println("  D<%>    duty cycle de 0 a 100 (ex.: D50)");
    Serial.println("  S       mostrar configuracao atual");
  }

  void handleCommand()
  {
    if (!Serial.available())
      return;

    const char command = static_cast<char>(toupper(Serial.read()));
    const long value = Serial.parseInt();

    if (command == 'F' && value >= static_cast<long>(MinFrequency) &&
        value <= static_cast<long>(MaxFrequency))
    {
      frequency = static_cast<uint32_t>(value);
      applyPwm();
      printStatus();
    }
    else if (command == 'D' && value >= 0 && value <= 100)
    {
      dutyPercent = static_cast<uint8_t>(value);
      applyPwm();
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

  for (uint8_t channel = 0; channel < ChannelCount; ++channel)
    ledcAttachPin(OutputPins[channel], channel);

  applyPwm();
  Serial.println("Gerador PWM ESP32 pronto: 16 canais, nivel de 3,3 V.");
  printStatus();
  printHelp();
}

void loop()
{
  handleCommand();
}