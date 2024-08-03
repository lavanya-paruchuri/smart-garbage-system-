#include <SoftwareSerial.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define TRIGGER_PIN 2
#define ECHO_PIN 3
#define MOTION_PIN 4
#define SERVO_PIN 5

SoftwareSerial gsmSerial(6, 7); // RX, TX for GSM module
Servo servoMotor;

const int WASTE_LEVEL_THRESHOLD = 50; // Set the waste level threshold (example: 50%)
bool dustbinFull = false;

char auth[] = "your_blynk_auth_token"; // Replace with your Blynk authentication token
char ssid[] = "your_wifi_ssid"; // Replace with your Wi-Fi SSID
char pass[] = "your_wifi_password"; // Replace with your Wi-Fi password

BlynkTimer timer;

void setup() {
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(MOTION_PIN, INPUT);

  servoMotor.attach(SERVO_PIN);

  gsmSerial.begin(9600);
  Serial.begin(9600);

  // Initialize the servo motor to closed position
  servoMotor.write(0);
  delay(1000);

  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, sendWasteLevelToBlink);
}

void loop() {
  Blynk.run();
  timer.run();
}

void sendWasteLevelToBlink() {
  int wasteLevel = getWasteLevel();
  Blynk.virtualWrite(V1, wasteLevel);
  
  if (wasteLevel >= WASTE_LEVEL_THRESHOLD && !dustbinFull) {
    dustbinFull = true;
    sendSMS("Dustbin is full. Please collect the waste.");
    openDustbinCap();
  }
  
  if (dustbinFull && !motionDetected()) {
    delay(5000); // Wait for 5 seconds to ensure no motion is detected
    if (!motionDetected()) {
      closeDustbinCap();
      dustbinFull = false;
    }
  }
}

int getWasteLevel() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2;
  int wasteLevel = map(distance, 5, 30, 0, 100); // Adjust the mapping based on your setup
  
  Serial.print("Waste Level (%): ");
  Serial.println(wasteLevel);
  
  return wasteLevel;
}

bool motionDetected() {
  return digitalRead(MOTION_PIN) == HIGH;
}

void openDustbinCap() {
  servoMotor.write(90); // Open position (adjust if necessary)
  delay(1000); // Wait for the cap to open
}

void closeDustbinCap() {
  servoMotor.write(0); // Closed position (adjust if necessary)
  delay(1000); // Wait for the cap to close
}

void sendSMS(String message) {
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  
  gsmSerial.println("AT+CMGS=\"+1234567890\""); // Replace with the recipient's phone number
  delay(1000);
  
  gsmSerial.println(message);
  delay(1000);
  
  gsmSerial.println((char)26);
  delay(1000);
  
  Serial.println("SMS sent");
}
