#include <Arduino.h>
#include <unity.h>

namespace
{
    constexpr uint8_t PwmResolution = 8;
    constexpr uint32_t TestFrequency = 1000;
    constexpr uint8_t TestChannel = 0;
    constexpr uint8_t TestPin = 4;
}

void setUp() {}
void tearDown() {}

void test_pwm_channel_can_be_configured()
{
    const uint32_t configuredFrequency = ledcSetup(
        TestChannel, TestFrequency, PwmResolution);
    ledcAttachPin(TestPin, TestChannel);
    ledcWrite(TestChannel, 128);

    TEST_ASSERT_EQUAL_UINT32(TestFrequency, configuredFrequency);
}

void setup()
{
    delay(500);
    UNITY_BEGIN();
    RUN_TEST(test_pwm_channel_can_be_configured);
    UNITY_END();
}

void loop() {}