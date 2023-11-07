#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "esp_bt_main.h"
#include "esp_wifi.h"

#include <Wire.h>
#include <Adafruit_BME280.h>

#define BATTERY_IO  34

#define MSENSOR_N 5
#define MSENSOR_IO 35

#define SOIL_POWER_PIN 17
#define BME280_POWER_PIN 5

#define MIN_MOISTURE 0 // 14-bit ADC range
#define MAX_MOISTURE 4095 // 14-bit ADC range
#define MMIN (MIN_MOISTURE) // + 4*180)
#define MMAX (MAX_MOISTURE) // - 4*440)

#define uS_TO_S_FACTOR 1000000  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  5 //(60*10)  // Time ESP32 will sleep (in seconds)

RTC_DATA_ATTR int32_t nBootCount = 0;
RTC_DATA_ATTR int64_t nSleepInterval = TIME_TO_SLEEP;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristicT = NULL;
BLECharacteristic* pCharacteristicSleep = NULL;

Adafruit_BME280 bme; // I2C
int32_t bBmePresent = 0;
bool bDeepSleep = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "2acd1159-5703-4b2d-95be-ac963748c40c"
#define CHARACTERISTIC_T_UUID "9b5099ae-10f0-4a78-a8b7-eb086e3cb69b"
#define CHARACTERISTIC_SLEEP_UUID "54021135-c289-4e63-af8e-653f32e7851a"

typedef struct {
  int32_t nBootCnt;
  float fTemperature;
  float fPressure;
  float fHumidity;
  float fBattery;
  float fSoilMoisture;
} M_DATA;

void deep_sleep()
{
  Serial.println("Deep Sleep");

//  esp_bluedroid_disable();
//  esp_bt_controller_disable();
//  esp_wifi_stop();
//  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_timer_wakeup(nSleepInterval* uS_TO_S_FACTOR);
  esp_deep_sleep_start();

  Serial.println("Shall not be printed");

}

//
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) 
  {
  };

  void onDisconnect(BLEServer* pServer) 
  {
    bDeepSleep = 1; //  deep_sleep();
  };
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) 
  {
    size_t len = pCharacteristic->getLength();
    if (len == sizeof(int32_t))
    {
      int32_t rxValue = *(int32_t*)pCharacteristic->getData();
      Serial.print("Received: ");
      Serial.println(rxValue);
      nSleepInterval = rxValue;
    }
//      esp_sleep_enable_timer_wakeup(nSleepInterval* uS_TO_S_FACTOR);
//      esp_deep_sleep_start();
    deep_sleep();
  };
};

//
void setup() 
{
  nBootCount++;

  Serial.begin(115200);

  Serial.println("Booting..."); //, check wiring or "

  pinMode(BME280_POWER_PIN, OUTPUT);
  pinMode(SOIL_POWER_PIN, OUTPUT);

  digitalWrite(BME280_POWER_PIN, HIGH);
  digitalWrite(SOIL_POWER_PIN, HIGH);

  int status = bme.begin(BME280_ADDRESS_ALTERNATE);
  if (!status) {
    Serial.println("No valid BME280 sensor"); //, check wiring or "
//                      "try a different address!"));
                      
//    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
//    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
//    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
//    Serial.print("        ID of 0x60 represents a BME 280.\n");
//    Serial.print("        ID of 0x61 represents a BME 680.\n");
//    while (1) delay(10);
    
  }
  else {
    bBmePresent = 1;
    // Default settings from datasheet. 
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,   // Operating Mode. 
                  Adafruit_BME280::SAMPLING_X2,     // Temp. oversampling 
                  Adafruit_BME280::SAMPLING_X16,    // Pressure oversampling 
                  Adafruit_BME280::SAMPLING_X16,    // Humidity oversampling 
                  Adafruit_BME280::FILTER_X16,      // Filtering. 
                  Adafruit_BME280::STANDBY_MS_500); // Standby time. 
  }

  // Create the BLE Device
  BLEDevice::init("W55");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID), 256, 0);

  // Create a BLE Characteristic
  pCharacteristicT = pService->createCharacteristic(
                      CHARACTERISTIC_T_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a BLE Characteristic
  pCharacteristicSleep = pService->createCharacteristic(
                      CHARACTERISTIC_SLEEP_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  BLEDescriptor* pDescT = new BLEDescriptor((uint16_t)0x2901);
  pDescT->setValue("M-Sensor Readings");
  BLE2902* pBLE2902_T = new BLE2902();
  pBLE2902_T->setNotifications(true);
  pCharacteristicT->addDescriptor(pDescT);
  pCharacteristicT->addDescriptor(pBLE2902_T);
  
  // Create a BLE Descriptor
  BLEDescriptor* pDescSleep = new BLEDescriptor((uint16_t)0x2901);
  pDescSleep->setValue("Deep Sleep (seconds)");
  BLE2902* pBLE2902_Sleep = new BLE2902();
  pBLE2902_Sleep->setNotifications(true);
  pCharacteristicT->addDescriptor(pDescSleep);
  pCharacteristicSleep->addDescriptor(pBLE2902_Sleep);

  pCharacteristicSleep->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

  M_DATA data = {nBootCount, 0, 0, 0, 0, 0};

  if (bBmePresent) 
  {
    data.fTemperature = bme.readTemperature();
    data.fPressure = bme.readPressure() / 100;
    data.fHumidity = bme.readHumidity();
  }

  data.fBattery = 2.0 * 3.67 * analogRead(BATTERY_IO) / 4095;

  int msensor = 0;
  for (int i = 0; i < MSENSOR_N; i++)
  {
    msensor += analogRead(MSENSOR_IO);  // Read the analog value from sensor
    delay(100);
  }
  data.fSoilMoisture = map(msensor/MSENSOR_N, MMIN, MMAX, 100, 0); // map the 14-bit data to 8-bit data

  pCharacteristicT->setValue((uint8_t*)&data, sizeof(data));
  pCharacteristicSleep->setValue((uint8_t*)&nSleepInterval, sizeof(nSleepInterval));

  BLEDevice::startAdvertising(); // start advertising

  // wait a bit for client to read the data
  int32_t nCnt = 1000;
  while (!bDeepSleep && nCnt > 0)
  {
    delay(30);
    nCnt--;
  }
  // power off
  deep_sleep();
}

void loop() 
{
  // never gets here
}
