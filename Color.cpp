#include "Color.h"

// float interpolate_(float a, float b, float ratio) {
//     if (a > b) return (a - b) * pow(ratio,     2.2) + b;
//     else       return (b - a) * pow(1 - ratio, 2.2) + a;
// }

uint32_t Color::rgb256() const {
    return ((uint32_t)red256() << 16) | ((uint32_t)green256() << 8) | blue256();
}

Color Color::brightness(float ratio) const {
    Color result;
    result.red   = red   * ratio;
    result.green = green * ratio;
    result.blue  = blue  * ratio;
    return result;
}

Color Color::gamma(float g) const {
    Color result;
    result.red   = pow(red,   g);
    result.green = pow(green, g);
    result.blue  = pow(blue,  g);
    return result;
}

Color Color::interpolate(Color &color, float ratio) const {
    Color result;
    result.red   = red * ratio + color.red * (1 - ratio);
    result.green = green * ratio + color.green * (1 - ratio);
    result.blue   = blue * ratio + color.blue * (1 - ratio);
    // result.red = interpolate_(red, color.red, ratio);
    // result.red   = (red   - color.red)   * ratio + color.red;
    // result.green = (green - color.green) * ratio + color.green;
    // result.blue  = (blue  - color.blue)  * ratio + color.blue;

    return result;
}

void Color::print() const {
    Serial.print(red);
    Serial.print(" ");
    Serial.print(green);
    Serial.print(" ");
    Serial.print(blue);
    Serial.print(" ");
}

const Color Color::WHITE   = Color(1.0, 1.0, 1.0);
const Color Color::BLACK   = Color(0.0, 0.0, 0.0);
const Color Color::RED     = Color(1.0, 0.0, 0.0);
const Color Color::MAGENTA = Color(1.0, 0.0, 1.0);
const Color Color::ORANGE  = Color(1.0, 0.5, 0.0);
const Color Color::YELLOW  = Color(1.0, 1.0, 0.0);
const Color Color::GREEN   = Color(0.0, 1.0, 0.0);
const Color Color::CYAN    = Color(0.0, 1.0, 1.0);
const Color Color::BLUE    = Color(0.0, 0.0, 1.0);
