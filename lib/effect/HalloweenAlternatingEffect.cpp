#include "HalloweenAlternatingEffect.hpp"

HalloweenAlternatingEffect::HalloweenAlternatingEffect() : Effect() {}

static uint8_t decayedBrightness(uint8_t steps) {
  uint8_t brightness = 255;
  while (steps-- > 0 && brightness > 0) {
    brightness = brightness * 3 / 4;
  }
  return brightness;
}

CRGB HalloweenAlternatingEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                                        const StripDescription &strip,
                                        RadioPacket *setEffectPacket) const {
  UNUSED(setEffectPacket);

  static constexpr uint8_t depth_multiplier = 128;
  static constexpr uint8_t palette_size = 2;

  if (strip.led_count == 0) {
    return CRGB::Black;
  }

  const CHSV purple(HUE_PURPLE, 255, 255);
  const CHSV orange(HUE_ORANGE, 255, 255);
  uint16_t stripe_width =
      strip.led_count * depth_multiplier * palette_size / 10;
  if (stripe_width < depth_multiplier * palette_size) {
    stripe_width = depth_multiplier * palette_size;
  }

  if (strip.FlagEnabled(Tiny)) {
    led_index = 0;
    time_ms *= 2;
  }

  const uint16_t led_step = depth_multiplier * palette_size;
  const uint16_t cycle_length = stripe_width * palette_size;
  const uint16_t v_index =
      (static_cast<uint32_t>(led_index) * led_step + time_ms % cycle_length) %
      cycle_length;
  const uint8_t color_index = (v_index / stripe_width) % palette_size;
  const uint16_t color_amount = v_index % stripe_width;
  CHSV color = color_index == 0 ? purple : orange;
  if (stripe_width <= led_step) {
    return color;
  }

  const uint16_t last_led_position = stripe_width - led_step;
  const uint16_t tail_fade_start =
      last_led_position > led_step ? last_led_position - led_step : 0;
  if (color_amount >= last_led_position) {
    CHSV next_color = color_index == 0 ? orange : purple;
    color.v = 0;
    const uint16_t transition_position = color_amount - last_led_position;
    const uint8_t blend_amount = transition_position * 255 / (led_step - 1);
    return blend(color, next_color, blend_amount, SHORTEST_HUES);
  } else if (color_amount >= tail_fade_start) {
    const uint8_t tail_brightness =
        decayedBrightness(tail_fade_start / led_step);
    color.v = tail_brightness * (last_led_position - color_amount) / led_step;
  } else {
    const uint8_t fade_steps = color_amount / led_step;
    const uint16_t fade_position = color_amount % led_step;
    const uint8_t current_brightness = decayedBrightness(fade_steps);
    const uint8_t next_brightness = decayedBrightness(fade_steps + 1);
    color.v = current_brightness -
              (current_brightness - next_brightness) * fade_position / led_step;
  }
  return color;
}
