/* #include "FastLED.h" */
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <OneButton.h>

#include "Fluctuation.h"
#include "Color.h"


// Peripherals

#define LED_PIN  32

#define LED_BUILTIN_PIN 2
#define HOMESPAN_BUTTON_PIN 23
#define LED_NUM_BUTTON_PIN 33

Adafruit_NeoPixel leds(32, LED_PIN, NEO_GRB + NEO_KHZ800);

OneButton ledNumButton = OneButton(LED_NUM_BUTTON_PIN, true, true);


// Flame

#define FPS 50

Fluctuation image;

#define X_MAX 128
unsigned X      = 9;
unsigned X_POW2 = 16;

#define X_ADDR 0

// Control

enum class State {
  Sleep, Show, SetNum
    };

State state = State::Show;
unsigned long timer = 0;

void onClick();

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN_PIN,     OUTPUT);

  Serial.begin(115200);

  Serial.print("start\n");

  psramInit();
  log_d("Total PSRAM: %d", ESP.getPsramSize());
  log_d("Free PSRAM: %d", ESP.getFreePsram());

  leds.begin();

  ledNumButton.attachClick(onClick);

  randomSeed(millis());

  EEPROM.begin(1);
  X = EEPROM.read(X_ADDR);
  log_d("Read X: %d", X);
  if (X < 1 || X_MAX < X) X = 32;
  leds.updateLength(X);
  X_POW2 = 2;
  while (X_POW2 < X) X_POW2 *= 2;

  Serial.println("initializing...");
  if (!image.init(X_POW2)) Serial.println("Failed to initializing image");
  Serial.println("initialized");
}

Color primary   = Color(1.0, 0.3, 0.0).brightness(0.5);
Color secondary = Color(1.0, 0.1, 0.0).brightness(0.005);


float p_ratio[X_MAX];

#define BLUR_LR 0.1
#define BLUR_C  0.8

void loop() {
  long start = millis();

  ledNumButton.tick();

  switch (state) {
  case State::Show: {
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
      /* float ratio = xf; */
      /* float ratio = (value[(x * 2 - 1) % (X * 2)] + */
      /*                value[x * 2] + */
      /*                value[(x * 2 + 1) % (X * 2)]) / 3; */

      ratio = 1.0 / (1.0 + exp(- ratio)); // Sigmoid
      ratio = pow(ratio, 2.2); // Gamma correction

      Color color = primary.interpolate(secondary, (ratio + p_ratio[x]) / 2.0);
      leds.setPixelColor(x, color.rgb256());

      p_ratio[x] = ratio;
    }

    /* Serial.println(" "); */

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

    timer++;

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
      state = State::Show;
    }

    break;
  default:
    Serial.println("other state");
  }

  long busy = millis() - start;
  /* Serial.println(busy / (1000.0 / FPS)); */
  int wait = (1000 / FPS) - busy;
  if (wait > 0) delay(wait);
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
