#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char *ssid = "IoT"; // Enter your WiFi name
const char *password = "iot2024!";  // Enter your WiFi password

// MQTT Broker details
const char *mqtt_broker = "broker.emqx.io";
const char *solenoid_topic = "solenoid_info"; // Topic for solenoid status
const char *mqtt_username = "akuaponik";
const char *mqtt_password = "akuaponik";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);
bool loop_enabled = true;
int lastD1State = LOW;

void setup() {
    // Initialize serial communication at 115200 baud
    Serial.begin(115200);
    // Start connecting to WiFi
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi network");

    // Setup MQTT client
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);

    // Connect to the MQTT broker
    reconnect();

    // Configure pin D1 as an output pin
    pinMode(D1, OUTPUT);
}

void callback(char* topic, byte* payload, unsigned int length) {
    String incomingMessage;
    for (int i = 0; i < length; i++) {
        incomingMessage += (char)payload[i];
    }
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(incomingMessage);

    // Check if the message is from the solenoid_topic
    if (String(topic) == solenoid_topic) {
        // Control the digital output based on the received message
        if (incomingMessage == "{\"idkolam\":\"7\",\"solenoid\":\"1\"}") {
            digitalWrite(D1, LOW); // Turn on the pin
            Serial.println("Pin D1 turned ON");
            loop_enabled = false; // Disable the loop
        } else if (incomingMessage == "{\"idkolam\":\"7\",\"solenoid\":\"0\"}") {
            digitalWrite(D1, HIGH);  // Turn off the pin
            Serial.println("Pin D1 turned OFF");
            loop_enabled = true; // Enable the loop
        }

        // Send solenoid status message
        // sendSolenoidStatus();
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop(); // Ensure this is called as often as possible

    // Only run the loop code if loop_enabled is true
    if (loop_enabled) {
        // Read the analog value from analog pin A0
        int sensorValue = analogRead(A0);

        // Convert the sensor value to a JSON formatted string for MQTT publishing
        char sensorValueStr[128];
        snprintf(sensorValueStr, sizeof(sensorValueStr), "{\"idsensor\": \"2\", \"value\": \"%d\"}", sensorValue);

        // Publish the sensor value to the MQTT topic
        client.publish("sensor/mac", sensorValueStr); // Publish to the original sensor/mac topic

        // Print the sensor value to the Serial Monitor
        Serial.print("Part Per Millions (PPM): ");
        Serial.println(sensorValue);

        // Control the digital output based on the sensor value
        if (sensorValue < 150) { // Adjust this threshold as needed
            digitalWrite(D1, HIGH); 
        } else {
            digitalWrite(D1, LOW); 
        }

        // Check if D1 state has changed
        int currentD1State = digitalRead(D1);
        if (currentD1State != lastD1State) {
            // sendSolenoidStatus();
            lastD1State = currentD1State;
        }

        // Short delay for debouncing (optional, can be fine-tuned)
        delay(60000);
    }
}

void reconnect() {
    while (!client.connected()) {
        String client_id = "esp8266-client-";
        client_id += String(ESP.getChipId()); // Use ESP8266 chip ID as client ID
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to public EMQX MQTT broker");
            // Subscribe to the solenoid topic after connecting
            client.subscribe(solenoid_topic);
            Serial.printf("Subscribed to solenoid topic %s\n", solenoid_topic);
        } else {
            Serial.print("Failed with state ");
            Serial.println(client.state());
            delay(2000);
        }
    }
}

// void sendSolenoidStatus() {
//     int solenoidState = digitalRead(D1);
//     char solenoidStatusStr[128];
//     if (solenoidState == HIGH) {
//         snprintf(solenoidStatusStr, sizeof(solenoidStatusStr), "{\"idkolam\":\"7\",\"solenoid\":\"0\"}");
//     } else {
//         snprintf(solenoidStatusStr, sizeof(solenoidStatusStr), "{\"idkolam\":\"7\",\"solenoid\":\"1\"}");
//     }
//     client.publish(solenoid_topic, solenoidStatusStr);
//     Serial.print("Solenoid status published: ");
//     Serial.println(solenoidStatusStr);
// }
