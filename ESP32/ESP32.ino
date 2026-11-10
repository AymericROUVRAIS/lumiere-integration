#include <FastLED.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <RTClib.h>

#define NUM_LEDS 16
#define DIN_PIN 2
#define DEFT_BRIGHTNESS 255

#define SDA_PIN 21
#define SCL_PIN 22

// Usage of libraries
CRGB leds[NUM_LEDS];
BluetoothSerial SerialBT;
RTC_DS3231 rtc;

// Variables
bool scheduleEnabled = false;

int scheduledHour = 8;
int scheduledMinute = 0;

int lastTriggeredDay   = -1;
int lastTriggeredMonth = -1;
int lastTriggeredYear  = -1;


// Custom functions
void setColor(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}

void ledsOff(void) {
  FastLED.clear();
  FastLED.show();
}

// For BT
bool isDigits(String value) {
  if (value.length() == 0) return false;

  for (int i = 0; i < value.length(); i++) {
    if (!isDigit(value[i])) return false;
  }

  return true;
}

void manageBrightness(String command) {
  String inputBrightness = command.substring(5); // 4 = len(BGNS )

  if (!isDigits(inputBrightness)) {
    SerialBT.println("Usage : BGNS value");
    return;
  }

  int brightnessValue = inputBrightness.toInt();

  FastLED.setBrightness(brightnessValue);
  FastLED.show();

}

void handleAdd(String command) {
  String timeValue = command.substring(4);

  int separator = timeValue.indexOf(':');

  if (separator == -1) {
    SerialBT.println("Usage: ADD H:M");
    return;
  }

  String h = timeValue.substring(0, separator);
  String m = timeValue.substring(separator + 1);

  if (!isDigits(h) || !isDigits(m)) {
    SerialBT.println("Invalid time format");
    return;
  }

  int hour = h.toInt();
  int minute = m.toInt();

  if (hour > 23 || minute > 59) {
    SerialBT.println("Invalid time");
    return;
  }

  scheduledHour = hour;
  scheduledMinute = minute;

  scheduleEnabled = true;

  lastTriggeredDay   = -1;
  lastTriggeredMonth = -1;
  lastTriggeredYear  = -1;

  SerialBT.printf("Schedule added: %02d:%02d\n", hour, minute);
}

void handleCommand(String command) {
  command.trim();
  command.toUpperCase();

  Serial.println("Received: " + command);

  if (command == "HELP" || command == "help") {
    SerialBT.println("Here are all the commands:");
    SerialBT.println("ADD H:M   -> to add a new alarm");
    SerialBT.println("REMOVE    -> to remove the alarm");
    SerialBT.println("SETTIME   -> set the time to when compiled");
    SerialBT.println("SET H:M   -> set the time to HH:MM");
    SerialBT.println("STATUS");

    SerialBT.println("For color configuration :");
    SerialBT.println("RED, GREEN, BLUE, ORANGE, PURPLE, WHITE");
    SerialBT.println("OFF");
  }
  else if (command.startsWith("ADD ")) handleAdd(command);
  else if (command == "REMOVE") {
    scheduleEnabled = false;
    SerialBT.println("Schedule removed");
  } else if (command == "STATUS") {
    DateTime now = rtc.now();

    SerialBT.printf(
      "Current time: %02d:%02d:%02d\n",
      now.hour(), now.minute(), now.second()
    );

    if (scheduleEnabled) {
      SerialBT.printf(
        "Scheduled: %02d:%02d\n",
        scheduledHour, scheduledMinute
      );
    } else {
      SerialBT.println("No active schedule");
    }
  }

  else if (command == "SETTIME") {
    // Set RTC to the time when this sketch was compiled.
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    SerialBT.println("RTC time set to sketch compilation time");
  }

  else if (command == "RED")    setColor(CRGB::Red);
  else if (command == "GREEN")  setColor(CRGB::Green);
  else if (command == "BLUE")   setColor(CRGB::Blue);
  else if (command == "WHITE")  setColor(CRGB::White);
  else if (command == "ORANGE") setColor(CRGB::LightYellow);
  else if (command == "YELLOW") setColor(CRGB::Yellow);
  else if (command == "PURPLE") setColor(CRGB::Purple);
  else if (command == "OFF")    ledsOff();

  else if (command.startsWith("BGNS ")) manageBrightness(command);
  else if (command == "RISE") {
    SerialBT.println("Starting...");

    setColor(CRGB::White);
    for (int brightness=1; brightness<254; brightness+=1){
      FastLED.setBrightness(brightness);
      FastLED.show();
      delay(10);
    }
    SerialBT.println("Full brightness");
  }

  else {
    SerialBT.println("Unknown command");
  }
}

void checkSchedule() {
  char brightness;

  if (!scheduleEnabled) return;

  DateTime now = rtc.now();

  if (now.hour() == scheduledHour &&
      now.minute() == scheduledMinute) {

    // if (lastTriggeredDay != now.day() ||
    //     lastTriggeredMonth != now.month() ||
    //     lastTriggeredYear != now.year()) {

      // Activate sun rise effect
      FastLED.setBrightness(10);
      setColor(CRGB::White);
      for (brightness=10;brightness<254; brightness+=1){
        FastLED.setBrightness(brightness);
        FastLED.show();
        delay(5);
      }

      // lastTriggeredDay = now.day();
      // lastTriggeredMonth = now.month();
      // lastTriggeredYear = now.year();

      SerialBT.println("Schedule triggered! LEDs ON");
    // }
  }
}


// Arduino code
void setup() {
  Serial.begin(115200);

  // Initialize LEDs
  FastLED.addLeds<WS2812B, DIN_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(DEFT_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("DS3231 not detected!");
    while (1) delay(100);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power! Send SETTIME after upload.");
  }

  // Initialize Bluetooth
  SerialBT.begin("Lumiere-ESP32");
  SerialBT.setTimeout(100);

  Serial.println("Bluetooth started!");
  Serial.println("RTC ready!");
}

void loop() {
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    handleCommand(command);
  }

  checkSchedule();

  delay(100);
}

