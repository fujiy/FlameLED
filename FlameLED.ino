/* #include "FastLED.h" */
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>
#include <HomeSpan.h>

#include "Fluctuation.h"
#include "Color.h"
#include "DEV_Identify.h"


// Peripherals

#define LED_PIN  32

#define HOMESPAN_LED_PIN 2
#define HOMESPAN_BUTTON_PIN 23
#define LED_NUM_BUTTON_PIN 33

Adafruit_NeoPixel leds(32, LED_PIN, NEO_GRB + NEO_KHZ800);
OneButton ledNumButton = OneButton(LED_NUM_BUTTON_PIN, true, true);


#define FPS 50
#define X_MAX 128
#define X_ADDR 0

#define BLUR_LR 0.1
#define BLUR_C  0.8
#define TRANSITION 0.1

enum class State { Sleep, Const, Flame, SetNum };

class FlameLED {
 private:
  // Control
  State state = State::Sleep;
  unsigned long timer = 0;
  long busy_max = 0;
  long busy_sum = 0;
  unsigned fade = 0;
  // FFT
  Fluctuation image;
  unsigned X      = 9;
  unsigned X_POW2 = 16;
  float p_ratio[X_MAX];
  // Color
  Color primary;
  Color secondary;
  Color brighter;
  Color darker;
  float brightness = 1.0;
  bool  primaryOn   = false;
  bool  secondaryOn = false;

 public:
  void init() {

    EEPROM.begin(1);
    X = EEPROM.read(X_ADDR);
    log_d("Read X: %d", X);
    if (X < 1 || X_MAX < X) X = 32;
    leds.updateLength(X);
    X_POW2 = 2;
    while (X_POW2 < X) X_POW2 *= 2;

    log_i("initializing...");
    if (!image.init(X_POW2)) log_e("Failed to initializing image");
    log_i("initialized");
  }

  void setPrimary(bool on, float brightness, Color color) {
    primary  = color;
    this->brightness = brightness;
    primaryOn = on;
    updateState();
  }
  void setSecondary(bool on, Color color) {
    secondary = color;
    secondaryOn = on;

    updateState();
  }
  void updateState() {
    State new_state = State::Sleep;
    if      (primaryOn && secondaryOn) new_state = State::Flame;
    else if (primaryOn)                new_state = State::Const;

    if (state != State::Sleep && new_state == State::Sleep) fade = FPS;
    else state = new_state;
  }

  void loop() {
    long start = micros();

    brighter = primary.interpolate(brighter, TRANSITION);
    darker   = secondary.brightness(brightness)
                  .interpolate(darker, TRANSITION);

    if (fade > 0) {
      fade--;
      if (fade == 0) state = State::Sleep;
    }

    switch (state) {
    case State::Sleep:
      if (timer % FPS == 0) {
        leds.clear();
        leds.show();
      }
      break;
    case State::Const:
      if (timer % FPS == 0) {
        leds.clear();
        for(int x = 0; x < X; x++) leds.setPixelColor(x, primary.rgb256());
        leds.show();
      }
      break;
    case State::Flame: {
      leds.clear();

      const double* value = image.next();
      /* image.printVec(value, X * 2); */

      for(int x = 0; x < X; x++) {

        float xr = (float)x / X * X_POW2;
        float xi;
        float xf = modff(xr, &xi);

        unsigned i = floor(xi);
        unsigned im1 = (i - 1) % X_POW2;
        unsigned ip1 = (i + 1) % X_POW2;
        unsigned ip2 = (i + 2) % X_POW2;

        float ratio =
          (value[im1] * (1-xf) + value[i]   * xf) * BLUR_LR +
          (value[i]   * (1-xf) + value[ip1] * xf) * BLUR_C +
          (value[ip1] * (1-xf) + value[ip2] * xf) * BLUR_LR;

        ratio = 1.0 / (1.0 + exp(- ratio)); // Sigmoid
        ratio = pow(ratio, 2.2); // Gamma correction

        Color color = brighter.interpolate(darker, (ratio + p_ratio[x]) / 2.0);
        leds.setPixelColor(x, color.rgb256());

        p_ratio[x] = ratio;
      }
      leds.show();
      break;
    }

    case State::SetNum:

      if (ledNumButton.isLongPressed() && timer % (FPS / 5) == 0) {
        if (X > 1) X--;
      }

      leds.clear();
      leds.setPixelColor(0,         Color::WHITE.rgb256());
      leds.setPixelColor(X - 1,     Color::WHITE.rgb256());
      leds.setPixelColor(timer % X, Color::ORANGE.rgb256());

      leds.show();

      if (timer > FPS * 5) {
        Serial.print("Set LED number: ");
        Serial.println(X);
        EEPROM.write(X_ADDR, X);
        EEPROM.commit();
        leds.updateLength(X);

        X_POW2 = 2;
        while (X_POW2 < X) X_POW2 *= 2;
        Serial.println(X_POW2);
        image.SetX(X_POW2);
        state = State::Flame;
      }
      break;
    }

    long busy = micros() - start;
    if (busy > busy_max) busy_max = busy;
    busy_sum += busy;

    if (timer % (FPS * 5) == 0) {
      log_d("busy (max, average): %3.1f%%, %3.1f%%",
            busy_max / (1000000.0 / FPS) * 100.0, busy_sum / 5000000.0 * 100.0);
      busy_sum = 0;
      busy_max = 0;
    }

    timer++;

    /* Serial.println(busy / (1000.0 / FPS)); */
    int wait = (1000000 / FPS) - busy;
    if (wait > 0) delayMicroseconds(wait);

  }

  void onClick() {
    Serial.println("clicked");
    if (state != State::SetNum) {
      state = State::SetNum;
      timer = 0;
    }
    else if (X < X_MAX) {
      X++;
      leds.updateLength(X);
    }
  }
};

struct DEV_Primary: Service::LightBulb {
  FlameLED &master;

  SpanCharacteristic *power;
  SpanCharacteristic *H;
  SpanCharacteristic *S;
  SpanCharacteristic *V;

 DEV_Primary(FlameLED &master): Service::LightBulb(), master(master) {
    power = new Characteristic::On();
    H = new Characteristic::Hue(0);
    S = new Characteristic::Saturation(0);
    V = new Characteristic::Brightness(100);
  }

  boolean update() {
    float h = H->getNewVal<float>();
    float s = S->getNewVal<float>();
    float v = V->getNewVal<float>();
    bool  p = power->getNewVal();

    master.setPrimary(p, v / 100, Color::HSV(h, s, v));

    return true;
  }
};

struct DEV_Secondary: Service::LightBulb {
  FlameLED &master;

  SpanCharacteristic *power;
  SpanCharacteristic *H;
  SpanCharacteristic *S;
  SpanCharacteristic *V;

 DEV_Secondary(FlameLED &master): Service::LightBulb(), master(master) {
    power = new Characteristic::On();
    H = new Characteristic::Hue(0);
    S = new Characteristic::Saturation(0);
    V = new Characteristic::Brightness(50);
  }

  boolean update() {
    float h   = H->getNewVal<float>();
    float s   = S->getNewVal<float>();
    float v   = V->getNewVal<float>();
    boolean p = power->getNewVal();

    master.setSecondary(p, Color::HSV(h, s, v));

    return true;
  }
};



FlameLED flameLED;

void onClick() {
  flameLED.onClick();
}

void setup() {
  Serial.begin(115200);

  psramInit();
  log_d("Total PSRAM: %d", ESP.getPsramSize());
  log_d("Free PSRAM: %d", ESP.getFreePsram());

  leds.begin();

  randomSeed(millis());

  flameLED.init();

  ledNumButton.attachClick(onClick);

  homeSpan.setControlPin(HOMESPAN_BUTTON_PIN);
  homeSpan.setStatusPin(HOMESPAN_LED_PIN);
  homeSpan.begin(Category::Bridges, "Flame LED Bridge");
  new SpanAccessory();
  new DEV_Identify("FlameLED Bridge","HomeSpan","123-ABC","FlameLED Bridge","0.9",3);
  new Service::HAPProtocolInformation();
  new Characteristic::Version("1.1.0");

  new SpanAccessory();
  (new DEV_Identify("Flame LED","HomeSpan","123-ABC","LED Strip","0.9",3))
    ->setPrimary();
  new DEV_Primary(flameLED);
  new Characteristic::Name("Primary Color");
  new DEV_Secondary(flameLED);
  new Characteristic::Name("Secondary Color");
}

void loop() {
  ledNumButton.tick();
  homeSpan.poll();
  flameLED.loop();
}

