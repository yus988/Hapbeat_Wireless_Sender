#ifndef COLOR_SENSOR_H
#define COLOR_SENSOR_H
#include "../../include/adjustParams.h"

namespace ColorSensor {
void initColorSensor();
void loopColorSensor();
void getColorValues(uint8_t& r, uint8_t& g, uint8_t& b);
String determineColor(uint8_t r, uint8_t g, uint8_t b);
}  // namespace ColorSensor

#endif