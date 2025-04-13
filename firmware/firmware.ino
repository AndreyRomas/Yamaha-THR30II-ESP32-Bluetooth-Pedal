#include <FastLED.h>
#include "BLEDevice.h"

// PINS
#define SWITCH_PIN 7
#define POWER_BUTTON_PIN GPIO_NUM_8
#define WS2812_DATA_PIN 9
#define BATTERY_VOLTAGE_PIN  3

// Config
#define NUM_LEDS 2
#define VOLTAGE_DIVIDER_RATIO 0.5 // Battery is connected to an analog pin to measure its voltage.
#define BLE_DEVICE_NAME "LE_THRII"
#define DEBUG false

// Voltage thresholds
#define VOLTAGE_THRESHOLD_LOW 3.3
#define VOLTAGE_THRESHOLD_MEDIUM 3.7

CRGB leds[NUM_LEDS];

BLEScan* pBLEScan;
BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCharacteristic;

static BLEUUID serviceUUID("03b80e5a-ede8-4b33-a751-6ce34ec4c700");
static BLEUUID charUUID("7772e5db-3868-4112-a1a9-f2669d106bf3");

byte currentPreset = 1;
bool currentSwitchState = LOW;

byte activateMessage1[]    = { 0x80, 0x80, 0xF0, 0x00, 0x01, 0x0C, 0x24, 0x02, 0x4D, 0x00, 0x01, 0x00, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xF7 };
byte activateMessage2[]    = { 0x80, 0x80, 0xF0, 0x00, 0x01, 0x0C, 0x24, 0x02, 0x4D, 0x00, 0x02, 0x00, 0x00, 0x03, 0x28, 0x72, 0x4D, 0x54, 0x5D, 0x00, 0x00, 0x00, 0x80, 0xF7 };
byte changePresetMessage[] = { 0x80, 0x80, 0xF0, 0x00, 0x01, 0x0C, 0x24, 0x02, 0x4D, 0x01, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xF7 };

void setup() {
  if (DEBUG) Serial.begin(57600);
  if (DEBUG) Serial.println("Start");

  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(POWER_BUTTON_PIN, INPUT);

  if (digitalRead(POWER_BUTTON_PIN) == LOW) {
    // Waiting for the power button to be released so it would not go into sleep mode again.
    waitForButtonRelease(POWER_BUTTON_PIN);
  } else {
    // Device was powered on due to battery insertion or USB connection, without a button press. It should not power up.
    powerOff();
  }

  FastLED.addLeds<NEOPIXEL, WS2812_DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  xTaskCreatePinnedToCore(backgroundTask, "backgroundTask", 10000, NULL, 1, NULL,  1);

  currentSwitchState = digitalRead(SWITCH_PIN);

  BLEDevice::init("ESP32_Client");

  initConnect();
}

void powerOff() {
  FastLED.clear();
  FastLED.show();

  // Waiting for the power button to be released so it would not wake up right after going into sleep mode.
  waitForButtonRelease(POWER_BUTTON_PIN);

  esp_sleep_enable_ext0_wakeup(POWER_BUTTON_PIN, 0);
  esp_deep_sleep_start();
}

void initConnect() {
  setStatusLED(CRGB::Orange);
  if (DEBUG) Serial.println("Init connect");

  while (!connectToTHR());

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

  if (DEBUG) Serial.println("Register for notify");

  pRemoteCharacteristic->writeValue((uint8_t*)activateMessage1, sizeof(activateMessage1), false);
  pRemoteCharacteristic->writeValue((uint8_t*)activateMessage2, sizeof(activateMessage2), false);

  if (DEBUG) Serial.println("Sent activate request");

  setStatusLED(CRGB::Blue);
}


void loop() {
  if (pRemoteCharacteristic && pClient->isConnected()) {
    if (currentSwitchState != digitalRead(SWITCH_PIN)) {
      currentPreset = 3 - currentPreset;
      sendChangePresetMessage(currentPreset);
      delay(100); // debouncing
      currentSwitchState = digitalRead(SWITCH_PIN);
    }
  } else {
    initConnect();
  }
}

bool connectToTHR() {
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  BLEScanResults* foundDevices = pBLEScan->start(5, false);

  for (int i = 0; i < foundDevices->getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices->getDevice(i);
    if (device.getName() == BLE_DEVICE_NAME) {
      BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

      pClient = BLEDevice::createClient();

      if (!pClient->connect(&device)) {
        if (DEBUG) Serial.println("Connect failed");
        return false;
      }

      BLERemoteService* pRemoteService = pClient->getService(serviceUUID);

      if (!pRemoteService) {
        if (DEBUG) Serial.println("Get service failed");
        return false;
      }

      pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);

      if (!pRemoteCharacteristic) {
        if (DEBUG) Serial.println("Get characteristic failed");
        return false;
      }

      pClient->setMTU(40);

      if (DEBUG) Serial.println("Connected");
      return true;
    }
  }

  if (DEBUG) Serial.println("Device not found");
  return false;
}

void backgroundTask(void *pvParameters) {
  unsigned long previousMillis = 0;
  const long interval = 1000;

  for (;;) {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      float voltage = analogReadMilliVolts(BATTERY_VOLTAGE_PIN) / VOLTAGE_DIVIDER_RATIO / 1000.0;

      if (voltage < VOLTAGE_THRESHOLD_LOW) {
        setBatteryLED(CRGB::Red);
      } else if (voltage >= VOLTAGE_THRESHOLD_LOW && voltage <= VOLTAGE_THRESHOLD_MEDIUM) {
        setBatteryLED(CRGB::Orange);
      } else {
        setBatteryLED(CRGB::Green);
      }

      FastLED.show();
    }

    if (digitalRead(POWER_BUTTON_PIN) == LOW) {
      powerOff();
    }
  }
}

void sendChangePresetMessage(byte presetNumber) {
  changePresetMessage[24] = presetNumber - 1;
  pRemoteCharacteristic->writeValue((uint8_t*)changePresetMessage, sizeof(changePresetMessage), false);
}

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (DEBUG) {
    for (size_t i = 0; i < length; i++) {
      Serial.print(pData[i], HEX);
      Serial.print(" ");
      if (pData[i] == 0xF7) Serial.println();
    }
  }
}

void waitForButtonRelease(int buttonPin) {
  while (digitalRead(buttonPin) == LOW);
  delay(100);
}

void setStatusLED(CRGB color) {
  leds[1] = color;
  FastLED.show();
}

void setBatteryLED(CRGB color) {
  leds[0] = color;
  FastLED.show();
}
