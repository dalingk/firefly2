#include "HalloweenCandyCornEffect.hpp"

HalloweenCandyCornEffect::HalloweenCandyCornEffect() : Effect() {}

CRGB HalloweenCandyCornEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                                     const StripDescription &strip,
                                     RadioPacket *setEffectPacket) const {
  (void)strip;
  (void)setEffectPacket;

  static constexpr uint8_t kUnitSize = 3;
  static constexpr uint32_t kTravelMs = 300u;

  const uint16_t cycle_len = kUnitSize * 3u;
  const uint16_t travel = (led_index + (time_ms / kTravelMs)) % cycle_len;
  const uint8_t segment = travel / kUnitSize;
  const uint8_t local = travel % kUnitSize;
  const uint8_t fade = cubicwave8((local * 255u) / (kUnitSize - 1u));

  const CHSV white(0, 0, 255);
  const CHSV orange(HUE_ORANGE, 255, 255);
  const CHSV yellow(HUE_YELLOW, 255, 255);

  if (segment == 0) {
    return blend(white, orange, fade, SHORTEST_HUES);
  }
  if (segment == 1) {
    return blend(orange, yellow, fade, SHORTEST_HUES);
  }
  return blend(yellow, white, fade, SHORTEST_HUES);
}
