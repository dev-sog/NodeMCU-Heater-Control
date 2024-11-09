#include "MQTTSetup.h"

// Initialize the client object
WiFiClient espClient;
PubSubClient client(espClient);

const char* mqtt_server = "broker.emqx.io";
const char* status_topic = "device1/SOG/heater/status";  // Only using status topic now
const char* subscribe_topic = "device1/SOG/heater/new_thresholds"; // Subscribe to receive new thresholds

unsigned long lastReconnectAttempt = 0;  // Track last reconnect attempt time
const unsigned long reconnectInterval = 5000;  // 5 seconds interval

void publishStatus(float temperature, bool heaterOn) {
    char statusMessage[150];  // Increased size to accommodate both status and thresholds
    snprintf(statusMessage, sizeof(statusMessage), 
             "{\"temperature\": %.2f, \"heater\": \"%s\", \"minTemp\": %.2f, \"maxTemp\": %.2f}", 
             temperature, heaterOn ? "ON" : "OFF", 
             thresholdsSet ? minTemp : NAN, 
             thresholdsSet ? maxTemp : NAN);
    Serial.print("Publishing status: ");
    Serial.println(statusMessage);
    if (client.publish(status_topic, statusMessage)) {
        Serial.println("Status published successfully!");
    } else {
        Serial.println("Failed to publish status.");
    }
}

void saveThresholdsToEEPROM() {
    EEPROM.put(0, minTemp);
    EEPROM.put(sizeof(float), maxTemp);
    EEPROM.commit();
    Serial.println("Thresholds saved to EEPROM.");
}

void loadThresholdsFromEEPROM() {
    EEPROM.get(0, minTemp);
    EEPROM.get(sizeof(float), maxTemp);
    if (!isnan(minTemp) && !isnan(maxTemp)) {
        thresholdsSet = true;
        Serial.println("Loaded thresholds from EEPROM.");
    } else {
        Serial.println("No valid thresholds in EEPROM.");
    }
}

void callback(char* topic, uint8_t* payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    
    DynamicJsonDocument doc(200);
    if (deserializeJson(doc, payload, length)) {
        Serial.println("Failed to deserialize JSON message.");
        return;
    }

    minTemp = doc["minTemp"] | NAN;
    maxTemp = doc["maxTemp"] | NAN;
    if (!isnan(minTemp) && !isnan(maxTemp)) {
        thresholdsSet = true;
        saveThresholdsToEEPROM();
    } else {
        Serial.println("Invalid threshold values received.");
    }
}

void setupMQTT() {
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    String clientId = "NodeMCUClient-" + WiFi.macAddress();
    clientId.replace(":", "");  // Remove colons for alphanumeric compliance
    Serial.print("Attempting MQTT connection with Client ID: ");
    Serial.println(clientId);

    if (client.connect(clientId.c_str())) {
        Serial.println("Connected to MQTT broker.");
        client.subscribe(subscribe_topic);
    } else {
        Serial.print("Initial MQTT connection failed, error code: ");
        Serial.println(client.state());
    }
    Serial.println("MQTT setup complete.");
}

bool reconnectMQTT() {
  String clientId = "NodeMCUClient-" + WiFi.macAddress();
    clientId.replace(":", "");

    Serial.print("Attempting MQTT reconnection with Client ID: ");
    Serial.println(clientId);

    if (client.connect(clientId.c_str())) {
        Serial.println("Reconnected to MQTT broker.");
        client.subscribe(subscribe_topic);
        return true;
    } else {
        Serial.print("Reconnection failed, error code: ");
        Serial.println(client.state());
        return false;
    }
}

// Non-blocking MQTT reconnection logic in the loop
void mqttLoop() {
    if (!client.connected()) {
        unsigned long currentMillis = millis();
        
        // Check if it's time to attempt reconnection
        if (currentMillis - lastReconnectAttempt > reconnectInterval) {
            lastReconnectAttempt = currentMillis;  // Update last attempt time
            
            // Try reconnecting to the MQTT broker
            if (reconnectMQTT()) {
                lastReconnectAttempt = 0;  // Reset on successful connection
            }
        }
    } else {
        client.loop();  // Maintain MQTT connection
    }
}
