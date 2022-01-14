
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "settings.h"
#include "OblastConnection.h"
#include <DHT.h>;

DHT dht(4, DHT22);



void reactor(String const& name, String const& value) {
  Serial.println("The reactor got an update:");
  Serial.print(name);
  Serial.print("=");
  Serial.print(value);
  Serial.println();
}
OblastVar temperature("fridge_temperature", reactor);
OblastVar humidity("fridge_humidity", reactor);

void setup() {
    // Setup serial port
    Serial.begin(9600);
    Serial.println();

    // Begin WiFi
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Connecting to WiFi...
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);

    // Loop continuously while WiFi is not connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Connected to WiFi
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());


    dht.begin();
  
    OblastConnection& connection = OblastConnection::getInstance();
    connection.connect();
    connection.subscribe(temperature);
      connection.subscribe(humidity);
}

void loop() {
    OblastConnection& connection = OblastConnection::getInstance();
    if(connection.isConnected()) {
        connection.handleUpdates();
        
        String temperatureString(dht.readTemperature());        
        temperature.setValue(temperatureString);
        
        String humidityString(dht.readHumidity());        
        humidity.setValue(humidityString);
        delay(1000);
    } else {
        Serial.println("not connected, waiting then trying to reconnect...");
        connection.connect(true); // connect with incremental retry
        if(connection.isConnected()) {
            connection.subscribe(temperature);
            connection.subscribe(humidity);
        }
    }
}
