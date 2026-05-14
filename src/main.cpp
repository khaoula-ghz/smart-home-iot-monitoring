#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <DHT_U.h>
#include "secrets.h" 

// Define MQTT topics
const char* publishing_TOPIC = "environment/pub";
const char* subscribing_TOPIC = "environment/sub";

// Sensor and pin definitions
#define LDR_PIN 34
#define RED_LED_PIN 12
#define GREEN_LED_PIN 13
#define BUZZER_PIN 14
#define ORANGE_LED_PIN 25
#define BLUE_LED_PIN 26
#define DHT_PIN 15
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

// Variables to hold temperature and humidity readings
String h;
String t;
String s;

// Wi-Fi and MQTT clients
WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

// Function to connect to Wi-Fi
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi!");
}

// Function to connect to AWS IoT Core
void connectToAWS() {
  wifiClient.setCACert(AWS_CERT_CA);
  wifiClient.setCertificate(AWS_CERT_CRT);
  wifiClient.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  while (!client.connected()) {
    Serial.println("Connecting to AWS IoT Core...");
    if (client.connect("EnvironmentalMonitorESP32")) {
      Serial.println("Successfully connected to AWS IoT Core!");
    } else {
      Serial.print("Connection failed, retrying. State: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
  // Subscribe to the single topic
  client.subscribe(subscribing_TOPIC);
}

// Setup function
void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize GPIO pins
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ORANGE_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  // Initialize the DHT sensor
  dht.begin();

  // Connect to Wi-Fi and AWS IoT Core
  connectToWiFi();
  connectToAWS();
}

// Function to send MQTT messages as JSON
void publishMessage() {
  // Create the JSON object once
  StaticJsonDocument<200> doc;
  doc["humidity"] = h;
  doc["temperature"] = t;
  doc["smoke"] = s;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);  // Convert data to JSON format

  if (client.publish(publishing_TOPIC, jsonBuffer)) {
    Serial.println("Message sent to topic " + String(publishing_TOPIC));
  } else {
    Serial.println("Failed to send message to topic " + String(publishing_TOPIC));
  }
}
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}


// Main loop
void loop() {
  if (!client.connected()) {
    connectToAWS();
  }
  client.loop();

  // Smoke detection logic
  int ldrValue = analogRead(LDR_PIN);
  s = (ldrValue < 500) ? "Smoke detected!" : "No smoke detected.";
  
  // Turn on Red LED and Buzzer if smoke is detected
  if (ldrValue < 500) {
    digitalWrite(RED_LED_PIN, HIGH);  // Red LED on (Smoke detected)
    digitalWrite(BUZZER_PIN, HIGH);   // Buzzer on
    digitalWrite(GREEN_LED_PIN, LOW); // Green LED off
  } else {
    digitalWrite(RED_LED_PIN, LOW);   // Red LED off
    digitalWrite(BUZZER_PIN, LOW);    // Buzzer off
    digitalWrite(GREEN_LED_PIN, HIGH); // Green LED on (No smoke)
  }

  // Temperature and humidity detection
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (!isnan(temperature) && !isnan(humidity)) {
    // Store temperature and humidity in the string variables
    h = String(humidity, 2);
    t = String(temperature, 2);

    // LED control based on temperature
    if (temperature > 30) {
      digitalWrite(BLUE_LED_PIN, HIGH); // Blue LED on (AC)
      digitalWrite(ORANGE_LED_PIN, LOW); // Orange LED off
    } else if (temperature < 10) {
      digitalWrite(ORANGE_LED_PIN, HIGH); // Orange LED on (Heating)
      digitalWrite(BLUE_LED_PIN, LOW);   // Blue LED off
    } else {
      digitalWrite(ORANGE_LED_PIN, LOW);  // Orange LED off
      digitalWrite(BLUE_LED_PIN, LOW);    // Blue LED off
    }

    // Publish the message
    publishMessage();

    // Print sensor readings to Serial Monitor
    Serial.println("Temp: " + String(temperature, 2) + "°C");
    Serial.println("Humidity: " + String(humidity, 2) + "%");
    Serial.println("Smoke: " + s);
    Serial.println("---");
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  delay(500);  // Wait for the next reading
}
