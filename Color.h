#include <Arduino.h>

class Color {
public:
    float red   = 0;
    float green = 0;
    float blue  = 0;

    Color() {};
    Color(float red, float green, float blue):
    red(red), green(green), blue(blue) {};


    uint8_t red256()   const { return constrain(red   * 255, 0, 255); };
    uint8_t green256() const { return constrain(green * 255, 0, 255); };
    uint8_t blue256()  const { return constrain(blue  * 255, 0, 255); };
    uint32_t rgb256() const;

    Color brightness(float ratio) const;
    Color gamma(float g) const;

    Color interpolate(Color &color, float ratio) const;

    void print() const;

    static Color RGB256(uint8_t red, uint8_t green, uint8_t blue);
    static Color HSV(float h, float s, float v);

    static const Color BLACK;
    static const Color WHITE;
    static const Color RED;
    static const Color MAGENTA;
    static const Color ORANGE;
    static const Color YELLOW;
    static const Color GREEN;
    static const Color CYAN;
    static const Color BLUE;
};

