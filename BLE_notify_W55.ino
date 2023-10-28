/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <Wire.h>
#include <Adafruit_BME280.h>

#define BATTERY_IO  34

#define MSENSOR_N 10
#define MSENSOR_IO 35

#define MIN_MOISTURE 0 // 14-bit ADC range
#define MAX_MOISTURE 4095 // 14-bit ADC range
#define MMIN (MIN_MOISTURE)// + 4*180)
#define MMAX (MAX_MOISTURE)// - 4*440)

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  20 //(60*10)        /* Time ESP32 will go to sleep (in seconds) */

#define MAX_CONNECT_WAIT  100 // in 100ms intervals

RTC_DATA_ATTR int bootCount = 0;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristicT = NULL;
//BLECharacteristic* pCharacteristicP = NULL;
//BLECharacteristic* pCharacteristicH = NULL;
//BLECharacteristic* pCharacteristicB = NULL;
//BLECharacteristic* pCharacteristicM = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

int32_t loopCnt = 0;

Adafruit_BME280 bme; // I2C
int bBmePresent = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "2acd1159-5703-4b2d-95be-ac963748c40c"
#define CHARACTERISTIC_T_UUID "9b5099ae-10f0-4a78-a8b7-eb086e3cb69b"
//#define CHARACTERISTIC_P_UUID "d2bc7047-ef44-4c5f-9913-3ee6a174e44a"
//#define CHARACTERISTIC_H_UUID "1c383a81-5387-41bc-b55f-3ff410da6f7e"
//#define CHARACTERISTIC_B_UUID "2af553d1-3f9f-4b19-ac6c-82ced3255cd0"
//#define CHARACTERISTIC_M_UUID "ea936dd5-eafa-4f70-9b7a-6a794a4de3a9"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(9600);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  int status = bme.begin(BME280_ADDRESS_ALTERNATE);
  if (!status) {
    Serial.println(F("No valid BME280 sensor")); //, check wiring or "
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

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristicT->addDescriptor(new BLE2902());
/*
  // Create a BLE Characteristic
  pCharacteristicP = pService->createCharacteristic(
                      CHARACTERISTIC_P_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a BLE Descriptor
  pCharacteristicP->addDescriptor(new BLE2902());

  // Create a BLE Characteristic
  pCharacteristicH = pService->createCharacteristic(
                      CHARACTERISTIC_H_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a BLE Characteristic
  pCharacteristicB = pService->createCharacteristic(
                      CHARACTERISTIC_B_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a BLE Characteristic
  pCharacteristicM = pService->createCharacteristic(
                      CHARACTERISTIC_M_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
*/
  // Create a BLE Descriptor
  pCharacteristicT->addDescriptor(new BLE2902());
//  pCharacteristicP->addDescriptor(new BLE2902());
//  pCharacteristicH->addDescriptor(new BLE2902());
//  pCharacteristicB->addDescriptor(new BLE2902());
//  pCharacteristicM->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // notify changed value
    if (deviceConnected) {
        float fT = 0;
        float fPa = 0;
        float fHu = 0;

        if (bBmePresent) {
          fT = bme.readTemperature();
          fPa = bme.readPressure() / 100;
          fHu = bme.readHumidity();
        }
        //Serial.print("T = ");
        //Serial.println(fT);
        //Serial.print("P = ");
        //Serial.println(fPa);
        //Serial.print("H = ");
        //Serial.println(fHu);

        float fV = 2.0 * 3.67 * analogRead(BATTERY_IO) / 4095;
        //Serial.println(fV);

        int msensor = 0;
        for (int i = 0; i < MSENSOR_N; i++)
        {
          msensor += analogRead(MSENSOR_IO);  // Read the analog value from sensor
          delay(100);
        }
        float fM = map(msensor/MSENSOR_N, MMIN, MMAX, 100, 0); // map the 14-bit data to 8-bit data

        float fNotify[] = {fT, fPa, fHu, fV, fM}; 
        pCharacteristicT->setValue((uint8_t*)&fNotify, sizeof(fNotify));

        delay(3000); 

        pCharacteristicT->notify();

/*
        pCharacteristicT->setValue((uint8_t*)&fT, sizeof(fT));
        pCharacteristicT->notify();
        pCharacteristicP->setValue((uint8_t*)&fPa, sizeof(fPa));
        pCharacteristicP->notify();
        pCharacteristicH->setValue((uint8_t*)&fHu, sizeof(fHu));
        pCharacteristicH->notify();
        pCharacteristicB->setValue((uint8_t*)&fV, sizeof(fV));
        pCharacteristicB->notify();
        pCharacteristicM->setValue((uint8_t*)&fM, sizeof(fM));
        pCharacteristicM->notify();
*/

        delay(500);
        
        esp_deep_sleep_start();
//        deviceConnected = false;

//        pServer->startAdvertising(); // restart advertising
//        Serial.println("start advertising");
    }
    else
    {
        // waiting for connection but not for too long
        delay(100); 
        if (loopCnt > MAX_CONNECT_WAIT)
        {
          esp_deep_sleep_start();
        }
        else
        {
          loopCnt++;
        }
    }
/*    
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(2000); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
*/    
}