#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <cmath>
#include <ESP32Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Define pins for sensors and actuator
#define DHT_PIN 15       // DHT22 sensor pin
#define LDR_PIN 33       // LDR sensor pin
#define SERVO_PIN 14     // Servo motor pin
#define BUZZER 12        //Buzzer pin

// WiFi and MQTT client objects
WiFiClient espClient;          // WiFi client
PubSubClient mqttClient(espClient); // MQTT client
DHTesp dhtSensor;            // DHT sensor object
Servo servoMotor;            // Servo motor object

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variable to store temperature data
char tempAr[6];

bool isScheduledON = false;
unsigned long scheduledOnTime;

// Unique client ID for MQTT
String clientId = "esp1254785458" + String(random(0, 1000));
const char* esp_client = clientId.c_str();

// Timing variables (in milliseconds)
unsigned long ldrSamplingInterval = 5000;  // LDR sampling interval (default 5 seconds)
unsigned long ldrSendingInterval = 120000; // LDR sending interval (default 2 minutes)
unsigned long tempSendingInterval = 2000;  // Temperature sending interval (2 seconds)

// LDR related variables
const int maxAdcValue = 4063;  // Maximum ADC reading
const int minAdcValue = 32;    // Minimum ADC reading
unsigned long ldrValueSum = 0;    // Sum of LDR readings
float normalizedLdrValue = 0.0; // Normalized LDR value

// Servo control variables
int servoMinimumAngle = 30;    // Minimum servo angle
float servoControlFactor = 0.75;  // Servo control factor
int idealMedicineTemperature = 30; // Ideal temperature for medicine

// MQTT topics
const char* ldrDataTopic = "220113V/LDR_DATA";         // LDR data topic
const char* tempDataTopic = "220113V/TEMP_DATA";        // Temperature data topic
const char* ldrReadingIntervalTopic = "220113V/reading_interval"; // LDR reading interval topic
const char* ldrTimePeriodTopic = "220113V/time_period";   // LDR time period topic
const char* servoMinimumAngleTopic = "220113V/minimum_angle";   // Minimum servo angle topic
const char* servoControlFactorTopic = "220113V/controlling_factor"; // Servo control factor topic
const char* idealTemperatureTopic = "220113V/ideal_temperature";  // Ideal temperature topic

// Setup function
void setup() {
  Serial.begin(115200);
  
  delay(2000);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  setupWiFiConnection();

  mqttClient.setCallback(mqttCallback);
  connectToMqttBroker();

  servoMotor.setPeriodHertz(50);
  servoMotor.attach(SERVO_PIN, 500, 2400);

  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);


  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}



void buzzerOn(bool on){
  if (on) {
    tone(BUZZER,256);
  }else{
    noTone(BUZZER);
  }
}

// Function to establish MQTT broker connection
void connectToMqttBroker() {
  const char* mqttBrokerAddress = "test.mosquitto.org";
  const int mqttBrokerPort = 1883;
  mqttClient.setServer(mqttBrokerAddress, mqttBrokerPort);
  mqttClient.setCallback(mqttCallback);


  int connectionAttempts = 0;
  const int maxConnectionAttempts = 5;

  while (!mqttClient.connected() && connectionAttempts < maxConnectionAttempts) {
    Serial.println("Attempting MQTT broker connection...");
    if (mqttClient.connect(esp_client)) {
      Serial.println("MQTT broker connected successfully");
      // Subscribe to relevant configuration topics
      mqttClient.subscribe(ldrReadingIntervalTopic);
      mqttClient.subscribe(ldrTimePeriodTopic);
      mqttClient.subscribe(servoMinimumAngleTopic);
      mqttClient.subscribe(servoControlFactorTopic);
      mqttClient.subscribe(idealTemperatureTopic);
      mqttClient.subscribe("ENTC-ADMIN-MAIN-ON-OFF");
      mqttClient.subscribe("ENTC-ADMIN-SCH-ON");

    } else {
      Serial.print("MQTT connection failed, reason code=");
      Serial.print(mqttClient.state());
      Serial.println(" - retrying in 2 seconds");
      delay(2000);
      connectionAttempts++;
    }
  }

  if (!mqttClient.connected()) {
    Serial.println("Failed to connect to MQTT broker after multiple retries");
    delay(10000);
  }
}



// Callback function for MQTT messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
    Serial.print((char)payload[i]);
  }
  message[length] = '\0';
  Serial.println();

  // Process incoming MQTT messages
  if (strcmp(topic, ldrReadingIntervalTopic) == 0) {
    ldrSamplingInterval = atoi(message) * 1000;
    Serial.print("LDR reading interval set to: ");
    Serial.println(ldrSamplingInterval);
  } else if (strcmp(topic, ldrTimePeriodTopic) == 0) {
    ldrSendingInterval = atof(message) * 60 * 1000;
    Serial.print("LDR time period set to: ");
    Serial.println(ldrSendingInterval);
  } else if (strcmp(topic, servoMinimumAngleTopic) == 0) {
    servoMinimumAngle = atoi(message);
    Serial.print("Minimum angle set to: ");
    Serial.println(servoMinimumAngle);
  } else if (strcmp(topic, servoControlFactorTopic) == 0) {
    servoControlFactor = atof(message);
    Serial.print("Controlling factor set to: ");
    Serial.println(servoControlFactor);
  } else if (strcmp(topic, idealTemperatureTopic) == 0) {
    idealMedicineTemperature = atoi(message);
    Serial.print("Ideal temperature set to: ");
    Serial.println(idealMedicineTemperature);
  } else if(strcmp(topic,"ENTC-ADMIN-MAIN-ON-OFF") == 0){
    if (message[0] == '1'){
      buzzerOn(true);
    }else{
      buzzerOn(false);
    }


  }else if(strcmp(topic,"ENTC-ADMIN-SCH-ON") == 0){
    if(message[0] == 'N'){
      isScheduledON = false;
    }else{
      isScheduledON = true;
      scheduledOnTime = atol(message);
    }
  }
  else {
    Serial.println("Unknown topic");
  }
}

// Function to calculate the servo position
double calculateServoAngle() {
  TempAndHumidity sensorData = dhtSensor.getTempAndHumidity();
  double currentTemperature = sensorData.temperature;

  double angle = servoMinimumAngle +
                 (180.0 - servoMinimumAngle) *
                 normalizedLdrValue *
                 servoControlFactor *
                 std::log((double)ldrSendingInterval / ldrSamplingInterval) *
                 (currentTemperature / idealMedicineTemperature);
  
  return constrain(angle, 0, 180);
}

// Function to setup WiFi connection
void setupWiFiConnection() {
  Serial.println();
  Serial.println("Connecting to ");
  Serial.println("Wokwi-Guest");
  WiFi.begin("Wokwi-GUEST", "");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

unsigned long getTime(){
  timeClient.update();
  return timeClient.getEpochTime();
}

void checkSchedule() {
  if(isScheduledON){
    unsigned long currenttime = getTime();
    if(currenttime > scheduledOnTime ){
      buzzerOn(true);
      isScheduledON = false;
      mqttClient.publish("ENTC-ADMIN-MAIN-ON-OFF-ESP", "1");
      mqttClient.publish("ENTC-ADMIN-SCH-ESP-ON", "0");
      Serial.println("Scheduled ON");
    }
  }

}


// Main loop
void loop() {
  if (!mqttClient.connected()) connectToMqttBroker();
  mqttClient.loop();

  // Read LDR sensor
  static unsigned long lastLdrSampleTime = 0;
  if (millis() - lastLdrSampleTime >= ldrSamplingInterval) {
    ldrValueSum += maxAdcValue - analogRead(LDR_PIN);
    lastLdrSampleTime = millis();
  }

  // Send LDR data
  static unsigned long lastLdrSendTime = 0;
  if (millis() - lastLdrSendTime >= ldrSendingInterval) {
    float sampleCount = (float)ldrSendingInterval / ldrSamplingInterval;
    float averageLdrValue = (float)ldrValueSum / sampleCount;
    normalizedLdrValue = averageLdrValue / (maxAdcValue - minAdcValue);
    
    char ldrMessage[12];
    dtostrf(normalizedLdrValue, 1, 4, ldrMessage);
    mqttClient.publish(ldrDataTopic, ldrMessage);
    ldrValueSum = 0;
    lastLdrSendTime = millis();
  }

  // Send temperature data
  static unsigned long lastTempSendTime = 0; 
  unsigned long currentTime = millis(); // Store current time to reduce calls to millis()

  if (currentTime - lastTempSendTime >= tempSendingInterval) {
    TempAndHumidity sensorData = dhtSensor.getTempAndHumidity(); // Get sensor data once
    dtostrf(sensorData.temperature, 1, 2, tempAr); // Convert temperature to char array

    mqttClient.publish(tempDataTopic, tempAr); // Publish data
    lastTempSendTime = currentTime; // Update last sent time
  }

  checkSchedule();


  // Update servo motor position periodically
  static unsigned long lastServoUpdateTime = 0;
  unsigned long CurrentTime = millis();
  const unsigned long servoUpdateInterval = 2000; // Update every 2 seconds

  if (CurrentTime - lastServoUpdateTime >= servoUpdateInterval) {
    lastServoUpdateTime = CurrentTime;
    int targetAngle = calculateServoAngle();
    int constrainedAngle = constrain(targetAngle, 0, 180);
    servoMotor.write(constrainedAngle);
    Serial.print("Servo angle updated to: ");
    Serial.println(constrainedAngle);
    Serial.print("normalizedLdrValue: ");
    Serial.println(normalizedLdrValue);
    
  }
}