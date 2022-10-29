#include <WiFi.h>
#include "settings.h"
#include "OblastConnection.h"
#include <SPL06-007.h>
#include "Adafruit_SHTC3.h"
#include <Wire.h>
#include "DHT.h"

#define DHT1PIN 16
#define DHTTYPE DHT22
DHT dht1(DHT1PIN, DHTTYPE);

Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

// Deep-sleep related constants:

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60*30        /* Time to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 0; // RTC memory is 8KB of data that is kept powered on during deeps-leep

// Oblast related constants:
void reactor(String const& name, String const& value) {
  // Nothing to do in case of value update...
}
OblastVar outside_spl06_temperature("outside_spl06_temperature", reactor);
OblastVar outside_spl06_pressure("outside_spl06_pressure", reactor);

OblastVar outside_shtc3_temperature("outside_shtc3_temperature", reactor);
OblastVar outside_shtc3_humidity("outside_shtc3_humidity", reactor);

OblastVar outside_dht22_temperature("outside_dht22_temperature", reactor);
OblastVar outside_dht22_humidity("outside_dht22_humidity", reactor);

void setup() {
  bootCount++;

  Serial.begin(115200);

  // Connecting to WiFi...
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());


  // Connecting to Oblast and subscribing variables:
  OblastConnection& connection = OblastConnection::getInstance();
  do {
      connection.connect(true);
     Serial.print("connecting...");
  } while(!connection.isConnected());
 Serial.print("connected");

  connection.subscribe(outside_spl06_temperature);
  connection.subscribe(outside_spl06_pressure);
  connection.subscribe(outside_shtc3_temperature);
  connection.subscribe(outside_shtc3_humidity);
  connection.subscribe(outside_dht22_temperature);
  connection.subscribe(outside_dht22_humidity);
  Serial.println("Oblast variables registered");

  // connection.handleUpdates(); // Not needed since we don't care reacting upon new update of our vars...

  // Starting I2C communication:
  Wire.begin();
  // a) Firing up the SPL06 sensor over I2C
  SPL_init();
  // b) Firing up SHTC3 sensor over I2C:
  if (! shtc3.begin()) {
    Serial.println("Couldn't find SHTC3");
    while (1) delay(1);
  }
  Serial.println("Found SHTC3 sensor");
  // c) Starting the DHT22 Sensor (over its custom DHT1PIN pin):
  dht1.begin();


  // SPL06 readings:
  outside_spl06_temperature.setValue(String(get_temp_c()).c_str());
  outside_spl06_pressure.setValue(String(get_pressure()).c_str());


  // SHTC3 readings:
  sensors_event_t shtc3_humidity, shtc3_temp;
  shtc3.getEvent(&shtc3_humidity, &shtc3_temp);// populate temp and humidity objects with fresh data
  outside_shtc3_temperature.setValue(String(shtc3_temp.temperature).c_str());
  outside_shtc3_humidity.setValue(String(shtc3_humidity.relative_humidity).c_str());

  // DHT22
  outside_dht22_temperature.setValue(String(dht1.readTemperature()).c_str());
  outside_dht22_humidity.setValue(String(dht1.readHumidity()).c_str());
  Serial.print("Humidity:"); Serial.println(String(dht1.readHumidity()).c_str());


  delay(1000);
  Serial.println("All sensor data are populated... Getting ready to sleep...");

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");
  Serial.flush();
  esp_deep_sleep_start(); // Deep-sleep now !
}

void loop() {
  // Do nothing in the loop, since the nodeMCU goes to deep-sleep at the end of the setup().
}
