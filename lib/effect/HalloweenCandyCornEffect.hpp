#ifndef __HALLOWEEN_CANDY_CORN_EFFECT_HPP__
#define __HALLOWEEN_CANDY_CORN_EFFECT_HPP__

#include "Effect.hpp"

class HalloweenCandyCornEffect : public Effect {
 public:
  HalloweenCandyCornEffect();

  CRGB GetRGB(uint8_t led_index, uint32_t time_ms,
              const StripDescription &strip,
              RadioPacket *setEffectPacket) const override;
};

#endif
