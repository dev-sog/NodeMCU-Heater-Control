#include <Arduino.h>
#include <DHT.h>
#include "WiFiSetup.h"
#include "OTAUpdate.h"
#include "MQTTSetup.h"
#include <EEPROM.h>

#define DHTPIN D4
#define DHTTYPE DHT11
#define HEATER_PIN D1

DHT dht(DHTPIN, DHTTYPE);

float minTemp = NAN;
float maxTemp = NAN;
bool heaterOn = false;
bool thresholdsSet = false;

unsigned long lastStatusTime = 0;
const unsigned long statusInterval = 10000;

void setup() {
    Serial.begin(115200);
    pinMode(HEATER_PIN, OUTPUT);
    digitalWrite(HEATER_PIN, LOW);
    dht.begin();

    EEPROM.begin(512);
    loadThresholdsFromEEPROM();
    
    connectWiFi();
    setupMQTT();  // Setup MQTT from MQTTSetup
    setupOTA();
}

void loop() {
    mqttLoop();  // Handle MQTT connection and loop

    ArduinoOTA.handle();  // Handle OTA updates

    float temperature = dht.readTemperature();
    if (isnan(temperature)) return;

    if (thresholdsSet) {
        if (temperature < minTemp && !heaterOn) {
            digitalWrite(HEATER_PIN, HIGH);
            heaterOn = true;
            publishStatus(temperature, heaterOn);  // Publish status from MQTTSetup
        } else if (temperature >= maxTemp && heaterOn) {
            digitalWrite(HEATER_PIN, LOW);
            heaterOn = false;
            publishStatus(temperature, heaterOn);  // Publish status from MQTTSetup
        }
    } else {
        Serial.println("Please set temperature thresholds to enable heater control.");
    }

    if (millis() - lastStatusTime >= statusInterval) {
        lastStatusTime = millis();
        publishStatus(temperature, heaterOn);  // Publish status from MQTTSetup
    }
}
