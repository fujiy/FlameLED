/* #include "FastLED.h" */
#include <Adafruit_NeoPixel.h>

#include "Fluctuation.h"
#include "Color.h"


/* float constrain(float x, float a, float b); */

#define LED_PIN  32

#define LED_BUILTIN 2

Adafruit_NeoPixel leds(32, LED_PIN, NEO_GRB + NEO_KHZ800);

#define FPS 50

Fluctuation image;

#define X_MAX 128
unsigned X = 32;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);

  Serial.print("start\n");


  psramInit();

  log_d("Total PSRAM: %d", ESP.getPsramSize());
  log_d("Free PSRAM: %d", ESP.getFreePsram());

  leds.begin();



  randomSeed(millis());
  image.SetX(X * 2);
  Serial.println("initializing...");
  if (!image.init()) Serial.println("Failed to initializing image");
  Serial.println("initialized");
  /* Serial.print("Mags:\n"); */
  /* image.printMags(); */
  /* Serial.print("Angles:\n"); */
  /* image.printAngles(); */
}

float p_ratio[X_MAX];

void loop() {

  long start = millis();

  const double* value = image.next();

  /* image.printVec(value, X * 2); */

  Color primary   = Color(1.0, 0.3, 0.0).brightness(0.5);
  Color secondary = Color(1.0, 0.1, 0.0).brightness(0.005);

  leds.clear();

  for(int x = 0; x < X; x++) {
    float ratio = (value[(x * 2 - 1) % (X * 2)] +
                   value[x * 2] +
                   value[(x * 2 + 1) % (X * 2)]) / 3;

    ratio = 1.0 / (1.0 + exp(- ratio)); // Sigmoid
    ratio = pow(ratio, 2.2); // Gamma correction


    /* ratio = (float)x / (image.GetX() - 1); */

    Color color = primary.interpolate(secondary, (ratio + p_ratio[x]) / 2.0);
    leds.setPixelColor(x, color.red256(), color.green256(), color.blue256());

    p_ratio[x] = ratio;
  }

  Serial.println("");

  leds.show();

  long busy = millis() - start;
  Serial.println(busy / (1000.0 / FPS));
  int wait = (1000 / FPS) - busy;
  if (wait > 0) delay(wait);
}

/* float constrain(float x, float a, float b) { */
/*   if      (x < a) x = a; */
/*   else if (x > b) x = b; */
/*   return x; */
/* } */
