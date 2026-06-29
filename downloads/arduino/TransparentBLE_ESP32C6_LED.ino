/*
  TransparentBLE ESP32-C6 LED profile sample

  Arduino IDE setup:
  - Board: an ESP32-C6 board from the ESP32 Arduino core
  - Library: Adafruit NeoPixel
  - Onboard WS2812/RGB LED pin: GPIO8

  BLE:
  - Device name: TransparentBLE-C6
  - Service: FFF0
  - Notify characteristic: FFF1
  - Write characteristic: FFF2

  Commands written to FFF2:
  - 01: read state
  - 02 00: LED off
  - 02 01: LED on, using current color
  - 03 RR GG BB: set RGB color and turn LED on
  - 04 BB: set brightness, 00-FF

  Every command sends a 12-byte notification on FFF1:
  A7 01 LED R G B BRI CMD UPTIME_SECONDS_BE32
*/

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

static constexpr uint8_t ledPin = 8;
static constexpr uint8_t ledCount = 1;

static const char *deviceName = "TransparentBLE-C6";
static const char *serviceUUID = "FFF0";
static const char *notifyUUID = "FFF1";
static const char *writeUUID = "FFF2";

Adafruit_NeoPixel pixel(ledCount, ledPin, NEO_GRB + NEO_KHZ800);
BLECharacteristic *notifyCharacteristic = nullptr;
bool clientConnected = false;

bool ledOn = false;
uint8_t red = 0x20;
uint8_t green = 0x60;
uint8_t blue = 0xFF;
uint8_t brightness = 0x20;
uint8_t lastCommand = 0x00;

void applyLED() {
  pixel.setBrightness(brightness);
  if (ledOn) {
    pixel.setPixelColor(0, pixel.Color(red, green, blue));
  } else {
    pixel.setPixelColor(0, 0);
  }
  pixel.show();
}

void notifyState(uint8_t command) {
  if (notifyCharacteristic == nullptr) {
    return;
  }

  lastCommand = command;
  const uint32_t uptimeSeconds = millis() / 1000;
  uint8_t packet[12] = {
    0xA7,
    0x01,
    ledOn ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0),
    red,
    green,
    blue,
    brightness,
    lastCommand,
    static_cast<uint8_t>((uptimeSeconds >> 24) & 0xFF),
    static_cast<uint8_t>((uptimeSeconds >> 16) & 0xFF),
    static_cast<uint8_t>((uptimeSeconds >> 8) & 0xFF),
    static_cast<uint8_t>(uptimeSeconds & 0xFF)
  };

  notifyCharacteristic->setValue(packet, sizeof(packet));
  notifyCharacteristic->notify();

  Serial.print("Notify: ");
  for (uint8_t byte : packet) {
    if (byte < 0x10) {
      Serial.print("0");
    }
    Serial.print(byte, HEX);
    Serial.print(" ");
  }
  Serial.println();
}

class CommandCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    auto value = characteristic->getValue();
    if (value.length() == 0) {
      return;
    }

    const uint8_t command = static_cast<uint8_t>(value[0]);
    switch (command) {
      case 0x01:
        break;

      case 0x02:
        if (value.length() >= 2) {
          ledOn = static_cast<uint8_t>(value[1]) != 0;
          applyLED();
        }
        break;

      case 0x03:
        if (value.length() >= 4) {
          red = static_cast<uint8_t>(value[1]);
          green = static_cast<uint8_t>(value[2]);
          blue = static_cast<uint8_t>(value[3]);
          ledOn = true;
          applyLED();
        }
        break;

      case 0x04:
        if (value.length() >= 2) {
          brightness = static_cast<uint8_t>(value[1]);
          applyLED();
        }
        break;

      default:
        break;
    }

    notifyState(command);
  }
};

class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    clientConnected = true;
    Serial.println("Client connected.");
  }

  void onDisconnect(BLEServer *server) override {
    clientConnected = false;
    Serial.println("Client disconnected. Restarting advertising.");
    delay(100);
    BLEDevice::startAdvertising();
  }
};

void setup() {
  Serial.begin(115200);
  delay(200);

  pixel.begin();
  applyLED();

  BLEDevice::init(deviceName);
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService *service = server->createService(serviceUUID);

  notifyCharacteristic = service->createCharacteristic(
    notifyUUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  notifyCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *writeCharacteristic = service->createCharacteristic(
    writeUUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  writeCharacteristic->setCallbacks(new CommandCallbacks());

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(serviceUUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("TransparentBLE ESP32-C6 sample is advertising.");
  Serial.println("Write 01 to FFF2 to read LED state.");
}

void loop() {
  delay(1000);
}
