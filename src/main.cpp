
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
  constexpr double MinFrequency = 0.5;
  constexpr double MaxFrequency = 150000.0;
  constexpr uint8_t MaxPwmResolution = 20;
  constexpr uint32_t LedcClockHz = 80000000;
  constexpr uint8_t LedPin = 2;

  const uint8_t OutputPins[] = {
      4, 5, 13, 14, 16, 17, 18, 19,
      21, 22, 23, 25, 26, 27, 32, 33};
  constexpr size_t ChannelCount = sizeof(OutputPins) / sizeof(OutputPins[0]);

  double frequency = 0.5;
  uint8_t dutyPercent = 50;
  uint8_t pwmResolution = 8;
  double actualFrequency = 0;
  bool slowMode = false;
  bool slowOutputState = false;
  uint32_t lastSlowToggle = 0;
  bool ledcAttached = false;

  uint8_t selectResolution()
  {
    uint8_t resolution = MaxPwmResolution;

    while (resolution > 1 &&
           frequency * (1ULL << resolution) > LedcClockHz)
      --resolution;

    return resolution;
  }

  void applyPwm()
  {
    slowMode = frequency < 1.0;

    if (slowMode)
    {
      if (ledcAttached)
      {
        for (const uint8_t pin : OutputPins)
          ledcDetachPin(pin);
        ledcDetachPin(LedPin);
        ledcAttached = false;
      }

      for (const uint8_t pin : OutputPins)
        pinMode(pin, OUTPUT);
      pinMode(LedPin, OUTPUT);

      slowOutputState = false;
      lastSlowToggle = millis();
      for (const uint8_t pin : OutputPins)
        digitalWrite(pin, LOW);
      digitalWrite(LedPin, LOW);
      actualFrequency = frequency;
      return;
    }

    pwmResolution = selectResolution();
    const uint32_t maxDuty = (1UL << pwmResolution) - 1;
    const uint32_t duty = (static_cast<uint32_t>(dutyPercent) * maxDuty) / 100;

    for (uint8_t channel = 0; channel < ChannelCount; ++channel)
    {
      ledcAttachPin(OutputPins[channel], channel);
      const double configuredFrequency = ledcSetup(channel, frequency, pwmResolution);
      if (channel == 0)
        actualFrequency = configuredFrequency;
      ledcWrite(channel, duty);
    }
    ledcAttachPin(LedPin, 0);
    ledcAttached = true;
  }

  void updateSlowPwm()
  {
    if (!slowMode)
      return;

    const uint32_t halfPeriod = static_cast<uint32_t>(500.0 / frequency);
    if (millis() - lastSlowToggle < halfPeriod)
      return;

    lastSlowToggle = millis();
    slowOutputState = !slowOutputState;
    const uint8_t level = slowOutputState ? HIGH : LOW;

    for (const uint8_t pin : OutputPins)
      digitalWrite(pin, level);
    digitalWrite(LedPin, level);
  }

  void printStatus()
  {
    Serial.printf("Frequencia: %.3f Hz (real: %.3f Hz) | Duty: %u%% | Canais: %u\n",
                  frequency, actualFrequency,
                  dutyPercent, static_cast<unsigned>(ChannelCount));
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

    if (command == 'F' && value >= MinFrequency && value <= MaxFrequency)
    {
      frequency = value;
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

  applyPwm();
  Serial.println("Gerador PWM ESP32 pronto: 16 canais, nivel de 3,3 V.");
  printStatus();
  printHelp();
}

void loop()
{
  updateSlowPwm();
  handleCommand();
}