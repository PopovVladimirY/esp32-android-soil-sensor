# ESP32 Wireless Soil Moisture Sensor

This Arduino project is based on ESP32 D1 mini board. Two sensors are connected. First, BME280 for temperature, air pressure and humidity readings. 
Second, the Capacitive Soil Moisture sensor. Battery level monitoring is added as well.

The ESP32 is in a deep sleep most of the time, waking up at programmable interval to read the sensors and make data available over BLE to a client for a few seconds before going off again. The BLE server is passive, letting the client read the data. This mode proved to be more reliable than BLE server-to-client notification when the server is off-line most of the time.

A client has an option to read and to change sensor deep sleep interval.

## No LEDs 

LEDs consume power. Power indicating LEDs are on even when the ESP32 controller is in a deep sleep mode. To save battery, no LEDs shall be on at any time when running on battery. 

>IMPORTANT: Remove power indicating LED from ESP32 module.

## Bill of materials

- ESP32 D1 mini board
- Capacitive soil moisture sensor
- BME280 sensor
- Battery, lithium-ion 502030 3.7v 250 mAh 
- Battery charging module
- Two 1 MOhm resistors
- Prototyping board, 3x7 cm
- Connector for battery
- Connector for soil sensor
- Connectors for ESP32 and BME280 modules

## Battery choice

Convinience of using 502030 battery model is that if fits exacly under the ESP D1 mini board. The capacity of 250 mAh shall be sufficient to keep the sensor alive for many months.

## Schematic

- Soil moisture sensor data is connected to IO35, and power to 3v3 of ESP32.
- BME280 I2C is connected to IO21 and IO22, and power to 3v3 of ESP32.
- Battery level sensor is at IO34 of ESP32.
- Battery positive pin is connected to battery charger 'B+' and to VCC  pin of ESP32.

## 3D printed case

The sensor case design can be found at the link:

    https://www.tinkercad.com/things/hqKOfJN0B2T-soil-wireless-2

It includes top and bottom parts as well as Reset button for ESP32.



