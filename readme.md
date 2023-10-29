# ESP32 Wireless Soil Moisture Sensor

This Arduino project is for ESP32 board. Two sensors are connected. First, BME280 for temperature, air pressure and humidity readings. 
Second, the Capacitive Soil Moisture sensor. Battery level monitoring is added as well.

The ESP32 is in a deep sleep most of the time, waking up at an interval to read the sensors and make data available over BLE to a client for a few seconds
before going off again. The BLE server is passive, letting the client read the data. This mode proved to be more reliable than BLE notification by client to a server.

A client has an option to read and to change sensor wakeup interval.


