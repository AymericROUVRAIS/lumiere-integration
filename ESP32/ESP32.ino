#include <FastLED.h>
#include <BluetoothSerial.h>

#define NUM_LEDS 16
#define DIN_PIN 2
#define BRIGHTNESS 1
#define DELAY_MS 500

CRGB leds[NUM_LEDS];
BluetoothSerial SerialBT;

void setup(){
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, DIN_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);   
  FastLED.clear();

  SerialBT.begin("Lumiere-ESP32");
  Serial.println("Bluetooth started!");
}

void loop(){
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();
    command.toUpperCase();

    Serial.println("Received: " + command);

    if (command == "ON") {
      fill_solid(leds, NUM_LEDS, CRGB::White);
      FastLED.show();
    } else if (command == "OFF") {
      FastLED.clear();
      FastLED.show();
    } else {
      SerialBT.println("Unknown command");
    }
  }
}
