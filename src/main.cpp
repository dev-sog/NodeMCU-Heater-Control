#include <Arduino.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h> // Include this for JSON parsing

#define DHTPIN D4         // Pin where the DHT11 data pin is connected
#define DHTTYPE DHT11     // DHT11 sensor type
#define HEATER_PIN D1     // Pin to control the heater relay

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "SOGTech";
const char* password = "D5hWk9Nc$QaLp";
const char* mqtt_server = "broker.emqx.io";

float minTemp = NAN;  // Minimum temperature threshold
float maxTemp = NAN;  // Maximum temperature threshold
bool heaterOn = false;
bool thresholdsSet = false;  // Flag to check if thresholds are set

unsigned long lastStatusTime = 0;   // Timer for periodic status update
const unsigned long statusInterval = 10000; // Interval for status update (1 min)

void connectWiFi();
void setupMQTT();
void reconnectMQTT();
void callback(char* topic, uint8_t* payload, unsigned int length);

void setup() {
    Serial.begin(115200);
    pinMode(HEATER_PIN, OUTPUT);
    digitalWrite(HEATER_PIN, LOW); // Ensure heater is off initially
    dht.begin();

    connectWiFi();
    setupMQTT();
}

void connectWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
}

void setupMQTT() {
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    reconnectMQTT();
}

void reconnectMQTT() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("NodeMCUClient")) {
            Serial.println("connected");
            client.subscribe("home/heater/thresholds"); // Single topic for both thresholds
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void callback(char* topic, uint8_t* payload, unsigned int length) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    
    if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return;
    }

    minTemp = doc["minTemp"] | NAN;
    maxTemp = doc["maxTemp"] | NAN;

    Serial.print("New minTemp set to: ");
    Serial.println(minTemp);
    Serial.print("New maxTemp set to: ");
    Serial.println(maxTemp); 

    if (!isnan(minTemp) && !isnan(maxTemp)) {
        thresholdsSet = true;
        Serial.println("Temperature thresholds set. Heater control enabled.");
    }
}

void publishStatus(float temperature) {
    char statusMessage[50];
    snprintf(statusMessage, sizeof(statusMessage), "{\"temperature\": %.2f, \"heater\": \"%s\"}", temperature, heaterOn ? "ON" : "OFF");
    client.publish("device/SOG/heater/status", statusMessage);
}

void loop() {
    if (!client.connected()) {
        reconnectMQTT();
    }
    client.loop();

    float temperature = dht.readTemperature();
    if (isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    if (thresholdsSet) {
        if (temperature < minTemp && !heaterOn) {
            digitalWrite(HEATER_PIN, HIGH);
            heaterOn = true;
            Serial.println("Heater turned ON");
            publishStatus(temperature);
        } else if (temperature >= maxTemp && heaterOn) {
            digitalWrite(HEATER_PIN, LOW);
            heaterOn = false;
            Serial.println("Heater turned OFF");
            publishStatus(temperature);
        }
    }

    unsigned long now = millis();
    if (now - lastStatusTime >= statusInterval) {
        lastStatusTime = now;
        publishStatus(temperature);
    }

    delay(100);
}
