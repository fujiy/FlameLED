#include <Arduino.h>

class Color {
public:
    float red   = 0;
    float green = 0;
    float blue  = 0;

    Color() {};
    Color(float red, float green, float blue):
    red(red), green(green), blue(blue) {};

    static Color Color256(uint8_t red, uint8_t green, uint8_t blue) {
        return Color(red / 255.0, green / 255.0, blue / 255.0);
    }

    uint8_t red256()   const { return constrain(red   * 255, 0, 255); };
    uint8_t green256() const { return constrain(green * 255, 0, 255); };
    uint8_t blue256()  const { return constrain(blue  * 255, 0, 255); };
    uint32_t rgb256() const;

    Color brightness(float ratio) const;
    Color gamma(float g) const;

    Color interpolate(Color &color, float ratio) const;

    void print() const;

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

