#include <DeviceDescription.hpp>
#include <LedManager.hpp>
#include <algorithm>
#include <vector>

#include "FakeLedManager.hpp"
#include "FakeRadio.hpp"
#include "HalloweenAlternatingEffect.hpp"
#include "gtest/gtest.h"

class EffectsTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}

  void runEffectsTest(uint32_t maxTime, uint32_t time_step = 1);

  FakeRadio radio;
  FireflyNetworkManager networkManager = FireflyNetworkManager(&radio);
  RadioStateMachine state_machine = RadioStateMachine(&networkManager);
  FakeLedManager* manager = nullptr;
};

// Make sure that all of the effects can run for a while without crashing
void EffectsTest::runEffectsTest(uint32_t maxTime, uint32_t time_step) {
  RadioPacket packet;
  std::vector<Effect*> ran_effects;
  for (uint8_t i = 0; i < manager->GetNumEffects(); i++) {
    packet.writeSetEffect(i, 0, 0);
    state_machine.SetEffect(&packet);
    Effect* effect = manager->GetCurrentEffect();

    // Effects are duplicated, make sure we haven't already tested this one.
    if (std::find(ran_effects.begin(), ran_effects.end(), effect) !=
        ran_effects.end()) {
      continue;
    }
    ran_effects.push_back(effect);

    for (uint32_t time = 0; time < maxTime; time += time_step) {
      setMillis(time);
      manager->RunEffect();
    }
  }
}

TEST_F(EffectsTest, oneLed) {
  StripDescription one_led_strip(1, {});
  DeviceDescription device = DeviceDescription(2000, {one_led_strip});
  manager = new FakeLedManager(device, &state_machine);
  runEffectsTest(60 * 1000);
  delete manager;
}

TEST_F(EffectsTest, hundredLeds) {
  StripDescription hundred_led_strip(100, {});
  DeviceDescription device = DeviceDescription(2000, {hundred_led_strip});
  manager = new FakeLedManager(device, &state_machine);
  runEffectsTest(60 * 1000);
  delete manager;
}

TEST_F(EffectsTest, allLedValues) {
  for (uint16_t num_leds = 0; num_leds < 256; num_leds++) {
    StripDescription strip(num_leds, {});
    DeviceDescription device = DeviceDescription(2000, {strip});
    manager = new FakeLedManager(device, &state_machine);
    ASSERT_NE(manager, nullptr) << "num_leds: " << num_leds;
    runEffectsTest(10 * 1000, 100);
    delete manager;
  }
}

TEST_F(EffectsTest, multipleStrips) {
  StripDescription twenty_led_strip(20, {});
  StripDescription ten_led_strip(10, {Tiny});
  StripDescription twelve_led_strip(12, {Circular});
  DeviceDescription device = DeviceDescription(
      2000, {twenty_led_strip, ten_led_strip, twelve_led_strip});
  EXPECT_EQ(device.GetLedCount(), 42);
  manager = new FakeLedManager(device, &state_machine);
  runEffectsTest(60 * 1000);
  delete manager;
}

TEST_F(EffectsTest, allColorPalettes) {
  StripDescription strip(20, {});
  DeviceDescription device = DeviceDescription(2000, {strip});
  manager = new FakeLedManager(device, &state_machine);

  RadioPacket packet;
  std::vector<Effect*> ran_effects;
  for (uint8_t effect_index = 0; effect_index < manager->GetNumEffects();
       effect_index++) {
    for (uint8_t palette_index = 0; palette_index < Effect::palettes().size();
         palette_index++) {
      packet.writeSetEffect(effect_index, /*delay=*/0, palette_index);
      state_machine.SetEffect(&packet);
      Effect* effect = manager->GetCurrentEffect();

      // Effects are duplicated, make sure we haven't already tested this one.
      if (std::find(ran_effects.begin(), ran_effects.end(), effect) !=
          ran_effects.end()) {
        continue;
      }
      ran_effects.push_back(effect);

      for (uint32_t time = 0; time < 60 * 1000; time += 10) {
        setMillis(time);
        manager->RunEffect();
      }
    }
  }
  delete manager;
}

// Golden spot-check pinning exact outputs so the switch to palette-by-reference
// (specs/002-fix-audit-findings, FR-005) remains provably output-identical.
// Rainbow/Color Cycle values include the intentional gradient power flattening
// from specs/004-fix-yellow-spike; Spark remains byte-identical to cceb41f.
TEST_F(EffectsTest, paletteByReferenceKeepsOutputIdentical) {
  RadioPacket packet;
  packet.writeSetEffect(0, 0, 8);
  StripDescription strip(36, {});
  RainbowEffect rainbow;
  ColorCycleEffect cycle;
  SparkEffect spark;

  const struct {
    const Effect* effect;
    uint8_t led;
    CRGB expected;
  } cases[] = {
      {&rainbow, 0, {40, 24, 0}},  {&rainbow, 17, {0, 0, 64}},
      {&rainbow, 35, {30, 34, 0}}, {&cycle, 0, {39, 24, 0}},
      {&cycle, 17, {39, 24, 0}},   {&cycle, 35, {39, 24, 0}},
      {&spark, 0, {0, 0, 0}},      {&spark, 17, {0, 0, 0}},
      {&spark, 35, {5, 8, 0}},
  };

  for (const auto& c : cases) {
    CRGB rgb = c.effect->GetRGB(c.led, 123456, strip, &packet);
    EXPECT_EQ(c.expected.r, rgb.r) << "led " << (int)c.led;
    EXPECT_EQ(c.expected.g, rgb.g) << "led " << (int)c.led;
    EXPECT_EQ(c.expected.b, rgb.b) << "led " << (int)c.led;
  }
}

TEST(HalloweenAlternatingEffectTest, BandsMoveAndTinyStripsShareOneColor) {
  HalloweenAlternatingEffect effect;
  RadioPacket packet;
  const auto same_rgb = [](const CRGB& lhs, const CRGB& rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
  };
  StripDescription fifty_led_strip(50, {});
  const CRGB first_band = effect.GetRGB(0, 0, fifty_led_strip, &packet);
  const CRGB second_band = effect.GetRGB(5, 0, fifty_led_strip, &packet);
  EXPECT_FALSE(same_rgb(first_band, second_band));
  EXPECT_TRUE(
      same_rgb(first_band, effect.GetRGB(10, 0, fifty_led_strip, &packet)));

  const CRGB full_brightness = CHSV(HUE_PURPLE, 255, 255);
  const CRGB three_quarter_brightness = CHSV(HUE_PURPLE, 255, 191);
  const CRGB nine_sixteenths_brightness = CHSV(HUE_PURPLE, 255, 143);
  const CRGB twenty_one_sixty_fourths_brightness = CHSV(HUE_PURPLE, 255, 107);
  const CRGB interpolated_tail_brightness = CHSV(HUE_PURPLE, 255, 53);
  const CRGB full_orange = CHSV(HUE_ORANGE, 255, 255);
  const CRGB three_quarter_orange = CHSV(HUE_ORANGE, 255, 191);
  EXPECT_TRUE(same_rgb(full_brightness, first_band));
  EXPECT_TRUE(same_rgb(full_orange, second_band));
  EXPECT_TRUE(same_rgb(three_quarter_brightness,
                       effect.GetRGB(1, 0, fifty_led_strip, &packet)));
  EXPECT_TRUE(same_rgb(nine_sixteenths_brightness,
                       effect.GetRGB(2, 0, fifty_led_strip, &packet)));
  EXPECT_TRUE(same_rgb(twenty_one_sixty_fourths_brightness,
                       effect.GetRGB(3, 0, fifty_led_strip, &packet)));
  EXPECT_TRUE(same_rgb(CHSV(HUE_PURPLE, 255, 106),
                       effect.GetRGB(3, 1, fifty_led_strip, &packet)));
  EXPECT_TRUE(
      same_rgb(CRGB::Black, effect.GetRGB(4, 0, fifty_led_strip, &packet)));
  EXPECT_TRUE(same_rgb(interpolated_tail_brightness,
                       effect.GetRGB(3, 128, fifty_led_strip, &packet)));
  const CRGB edge_blend = blend(CHSV(HUE_PURPLE, 255, 0),
                                CHSV(HUE_ORANGE, 255, 255), 128, SHORTEST_HUES);
  EXPECT_TRUE(
      same_rgb(edge_blend, effect.GetRGB(4, 128, fifty_led_strip, &packet)));
  EXPECT_TRUE(same_rgb(three_quarter_orange,
                       effect.GetRGB(6, 0, fifty_led_strip, &packet)));
  EXPECT_TRUE(same_rgb(effect.GetRGB(0, 256, fifty_led_strip, &packet),
                       effect.GetRGB(1, 0, fifty_led_strip, &packet)));
  EXPECT_TRUE(
      same_rgb(second_band, effect.GetRGB(0, 1280, fifty_led_strip, &packet)));

  StripDescription strip(48, {});

  const CRGB purple_band = effect.GetRGB(12, 0, strip, &packet);
  const CRGB moving_transition = effect.GetRGB(12, 1728, strip, &packet);
  EXPECT_FALSE(same_rgb(purple_band, moving_transition));
  EXPECT_FALSE(same_rgb(purple_band, effect.GetRGB(36, 0, strip, &packet)));

  StripDescription long_strip(255, {});
  EXPECT_TRUE(same_rgb(CRGB::Black, effect.GetRGB(20, 0, long_strip, &packet)));

  StripDescription tiny_strip(10, {Tiny});
  EXPECT_TRUE(same_rgb(effect.GetRGB(0, 1234, tiny_strip, &packet),
                       effect.GetRGB(9, 1234, tiny_strip, &packet)));
}
