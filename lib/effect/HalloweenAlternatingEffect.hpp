#ifndef __HALLOWEEN_ALTERNATING_EFFECT_HPP__
#define __HALLOWEEN_ALTERNATING_EFFECT_HPP__

#include "Effect.hpp"

class HalloweenAlternatingEffect : public Effect {
 public:
  HalloweenAlternatingEffect();

  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;
};

#endif
