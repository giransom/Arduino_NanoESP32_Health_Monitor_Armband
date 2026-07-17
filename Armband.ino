#include <Wire.h>

const int MPU_ADDR = 0x68;
const int motorPin = D9;
const int fsrPin = A0;
int16_t lastX = 0;
int16_t lastY = 0;
int16_t lastZ = 0;
bool mpuFound = false;
const int movementThreshold = 5000;
const int fsrThreshold = 500; // adjust this based on your FSR readings
void setup() {
    Serial.begin(115200);
    delay(3000);
    Serial.println("Step 6: Motion + FSR + vibration test starting...");
    pinMode(motorPin, OUTPUT);
    digitalWrite(motorPin, LOW);
    Wire.begin();
    // Check if MPU6050 is connected
    Wire.beginTransmission(MPU_ADDR);
    byte error = Wire.endTransmission();
    if (error == 0) {
        Serial.println("MPU6050 found!");
        mpuFound = true;
        // Wake up MPU6050
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x6B);
        Wire.write(0);
        Wire.endTransmission();
        readAccelerometer(lastX, lastY, lastZ);
    } else {
        Serial.println("MPU6050 not found. Check wiring.");
        mpuFound = false;
    }
}
void loop() {
    int fsrValue = analogRead(fsrPin);
    Serial.print("FSR: ");
    Serial.print(fsrValue);
    if (mpuFound) {
        int16_t x, y, z;
        readAccelerometer(x, y, z);
        int movement = abs(x - lastX) + abs(y - lastY) + abs(z - lastZ);
        Serial.print(" | Movement: ");
        Serial.println(movement);
        if (movement > movementThreshold) {
            Serial.println("Movement detected! Buzz!");
            buzzMotor();
        }
        lastX = x;
        lastY = y;
        lastZ = z;
    } else {
        Serial.println(" | No accelerometer detected.");
    }
    if (fsrValue > fsrThreshold) {
        Serial.println("FSR pressed! Buzz!");
        buzzMotor();
    }
    delay(300);
}
void readAccelerometer(int16_t & x, int16_t & y, int16_t & z) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 6, true);
    x = Wire.read() << 8 | Wire.read();
    y = Wire.read() << 8 | Wire.read();
    z = Wire.read() << 8 | Wire.read();
}
void buzzMotor() {
    digitalWrite(motorPin, HIGH);
    delay(300);
    digitalWrite(motorPin, LOW);
    delay(700);
}

