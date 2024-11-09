#ifndef MQTT_SETUP_H
#define MQTT_SETUP_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include "MQTTSetup.h"
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

// MQTT server details
extern const char* mqtt_server;
extern const char* status_topic;
extern const char* thresholds_topic;
extern const char* subscribe_topic;
extern bool thresholdsSet;
extern float minTemp;
extern float maxTemp;

// Global MQTT client object
extern PubSubClient client;
void setupMQTT();
bool reconnectMQTT();
void mqttLoop();
void publishStatus(float temperature, bool heaterOn);
void saveThresholdsToEEPROM();
void loadThresholdsFromEEPROM();
void callback(char* topic, uint8_t* payload, unsigned int length);

#endif 
