#include <Wire.h>                 // Lets Arduino talk to MPU6050 over I2C

#include <OneWire.h>              // Needed for DS18B20 temperature sensor

#include <DallasTemperature.h>    // Makes DS18B20 easier to read
 // -------------------- PINS --------------------
const int motorPin = D9; // Vibration motor signal pin
const int buzzerPin = D8; // Passive piezo buzzer pin
const int buttonPin = D2; // Button pin
const int tempPin = D3; // DS18B20 data pin
// -------------------- ACCELEROMETER --------------------
const int MPU_ADDR = 0x68; // Default address for MPU6050
bool accelFound = false; // Tracks whether MPU6050 was detected
int16_t baseX, baseY, baseZ; // Starting position of accelerometer
// -------------------- TEMPERATURE SENSOR --------------------
OneWire oneWire(tempPin); // Creates OneWire connection on tempPin
DallasTemperature tempSensor( & oneWire); // Creates DS18B20 sensor object
// -------------------- ALERT STATES --------------------
bool motionAlert = false; // True when movement alert is active
bool tempAlert = false; // True when temperature alert is active
bool alertsMuted = false; // True after button is pressed
// -------------------- THRESHOLDS --------------------
// Old raw threshold was 12000.
// 12000 / 16384 = about 0.73g
const float motionThresholdG = 0.73;
const float lowTempC = 35.0; // Too cold threshold
const float highTempC = 38.0; // Too hot threshold
// -------------------- SETUP --------------------
void setup() {
    Serial.begin(115200); // Start Serial Monitor
    delay(2000); // Wait so Serial Monitor can open
    pinMode(motorPin, OUTPUT); // Motor is controlled by Arduino
    pinMode(buzzerPin, OUTPUT); // Buzzer is controlled by Arduino
    pinMode(buttonPin, INPUT_PULLUP); // Button uses internal pull-up resistor
    digitalWrite(motorPin, LOW); // Start with motor off
    noTone(buzzerPin); // Start with buzzer off
    Serial.println("Patient alert project starting...");
    // Start DS18B20 temperature sensor
    tempSensor.begin();
    Serial.print("DS18B20 sensors found: ");
    Serial.println(tempSensor.getDeviceCount());
    // Start I2C communication for MPU6050
    Wire.begin();
    // Check if MPU6050 is connected
    Wire.beginTransmission(MPU_ADDR);
    byte error = Wire.endTransmission();
    if (error == 0) {
        Serial.println("MPU6050 found!");
        accelFound = true;
        // Wake up MPU6050 from sleep mode
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x6B); // Power management register
        Wire.write(0); // Set to 0 to wake up
        Wire.endTransmission();
        delay(500);
        // Save current position as the "normal" starting position
        readAccelerometer(baseX, baseY, baseZ);
    } else {
        Serial.println("MPU6050 not found. Motion alert disabled.");
        accelFound = false;
    }
}
// -------------------- MAIN LOOP --------------------
void loop() {
    // If button is clicked, stop/mute current alerts
    if (buttonPressed()) {
        clearAlerts(); // Clear alert states
        alertsMuted = true; // Keep alert muted after release
        Serial.println("Alerts muted by button.");
        delay(500); // Simple button debounce
        return;
    }
    checkMotion(); // Check accelerometer movement
    checkTemperature(); // Check DS18B20 temperature
    // If no alert is active anymore, allow future alerts again
    if (!motionAlert && !tempAlert) {
        alertsMuted = false;
    }
    // If user muted the alert, keep motor/buzzer off
    if (alertsMuted) {
        stopOutputs();
        delay(100);
        return;
    }
    // Pick alert pattern based on active alert type
    if (motionAlert && tempAlert) {
        playBothAlert(); // Continuous alert
    } else if (motionAlert) {
        playMotionAlert(); // Long beep pattern
    } else if (tempAlert) {
        playTempAlert(); // Double beep pattern
    } else {
        stopOutputs(); // No alerts, stay quiet
    }
    delay(100); // Small pause between checks
}
// -------------------- MOTION CHECK --------------------
void checkMotion() {
    if (!accelFound) return; // Skip if MPU6050 was not found
    int16_t x, y, z; // Current accelerometer readings
    readAccelerometer(x, y, z); // Get current X/Y/Z values
    // Compare current position to starting position
    int rawMovement = abs(x - baseX) + abs(y - baseY) + abs(z - baseZ);
    // Convert raw MPU6050 units to approximate g-force units
    float movementG = rawMovement / 16384.0;
    Serial.print("Movement G: ");
    Serial.println(movementG);
    // Trigger alert if movement is above threshold
    if (movementG > motionThresholdG) {
        motionAlert = true;
    }
}
// -------------------- TEMPERATURE CHECK --------------------
void checkTemperature() {
    tempSensor.requestTemperatures(); // Ask sensor for temperature
    float tempC = tempSensor.getTempCByIndex(0); // Read first sensor
    Serial.print("Temp C: ");
    Serial.println(tempC);
    // -127 means sensor is disconnected/not detected
    if (tempC == DEVICE_DISCONNECTED_C) {
        Serial.println("Temperature sensor not detected!");
        tempAlert = true;
        return;
    }
    // Trigger alert if temperature is outside safe range
    if (tempC < lowTempC || tempC > highTempC) {
        tempAlert = true;
    }
}
// -------------------- ALERT PATTERNS --------------------
void playMotionAlert() {
    // Pattern: beeeep ... beeeep ... beeeep
    digitalWrite(motorPin, HIGH); // Motor on
    tone(buzzerPin, 1000); // Buzzer on at 1000 Hz
    delay(600); // Long beep
    digitalWrite(motorPin, LOW); // Motor off
    noTone(buzzerPin); // Buzzer off
    delay(400); // Pause
}
void playTempAlert() {
    // Pattern: beepbeep ... beepbeep ... beepbeep
    for (int i = 0; i < 2; i++) {
        digitalWrite(motorPin, HIGH); // Motor on
        tone(buzzerPin, 1500); // Buzzer on at 1500 Hz
        delay(150); // Short beep
        digitalWrite(motorPin, LOW); // Motor off
        noTone(buzzerPin); // Buzzer off
        delay(150); // Short pause
    }
    delay(500); // Pause before repeating pattern
}
void playBothAlert() {
    // Pattern: continuous beeeeeeeeeep
    digitalWrite(motorPin, HIGH); // Motor stays on
    tone(buzzerPin, 2000); // Buzzer stays on at 2000 Hz
}
// -------------------- HELPER FUNCTIONS --------------------
void clearAlerts() {
    motionAlert = false; // Clear motion alert
    tempAlert = false; // Clear temperature alert
    stopOutputs(); // Turn off motor and buzzer
}
void stopOutputs() {
    digitalWrite(motorPin, LOW); // Turn motor off
    noTone(buzzerPin); // Turn buzzer off
}
bool buttonPressed() {
    // INPUT_PULLUP means:
    // not pressed = HIGH
    // pressed = LOW
    return digitalRead(buttonPin) == LOW;
}
void readAccelerometer(int16_t & x, int16_t & y, int16_t & z) {
    // Tell MPU6050 we want accelerometer data
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); // First accelerometer data register
    Wire.endTransmission(false);
    // Request 6 bytes total: X, Y, Z
    Wire.requestFrom(MPU_ADDR, 6, true);
    // Combine high byte and low byte for each axis
    x = Wire.read() << 8 | Wire.read();
    y = Wire.read() << 8 | Wire.read();
    z = Wire.read() << 8 | Wire.read();
}
