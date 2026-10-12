// ESP32 Air Mouse
// MPU6050 gyro -> BLE HID mouse, 2 buttons, live tuning over USB serial.
// Board: classic ESP32 DevKit, esp32 core 3.2.0
// Libraries: ESP32-BLE-Mouse by T-vK (patched, see README)

#include <Wire.h>
#include <BleMouse.h>
#include "esp_mac.h"

#define BAUD 115200
#define SDA_PIN   21
#define SCL_PIN   22
#define BTN_LEFT  19
#define BTN_RIGHT 18
#define MPU_ADDR  0x68

// ---- CONFIG (paste from the tuner) ----
int   X_AXIS   = 2;     // 0=gx 1=gy 2=gz
int   X_SIGN   = -1;
int   Y_AXIS   = 0;
int   Y_SIGN   = -1;
float SENS     = 0.35;
float DEADZONE = 2.5;
float SMOOTH   = 0.00;

BleMouse bleMouse("AirMouse2", "Aykhan", 100);

float biasX = 0, biasY = 0, biasZ = 0;
float accX = 0, accY = 0, smX = 0, smY = 0;
bool mpuAwake = false, streaming = false;
bool lastL = false, lastR = false;

void mpuWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg); Wire.write(val);
  Wire.endTransmission();
}

bool readGyro(float g[3]) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, 6) != 6) return false;
  for (int i = 0; i < 3; i++) {
    int16_t v = (Wire.read() << 8) | Wire.read();
    g[i] = v / 65.5;  // +-500 dps range
  }
  return true;
}

void calibrate() {
  Serial.println("Calibrating... keep it STILL");
  float s[3] = {0, 0, 0}, g[3];
  int n = 0;
  for (int i = 0; i < 500; i++) {
    if (readGyro(g)) { s[0] += g[0]; s[1] += g[1]; s[2] += g[2]; n++; }
    delay(3);
  }
  if (n == 0) { Serial.println("MPU NOT RESPONDING"); return; }
  biasX = s[0] / n; biasY = s[1] / n; biasZ = s[2] / n;
  Serial.println("Calibration done");
}

void wakeMPU() {
  if (mpuAwake) return;
  mpuWrite(0x6B, 0x00);  // wake
  mpuWrite(0x1A, 0x03);  // low pass ~44 Hz
  mpuWrite(0x1B, 0x08);  // gyro +-500 dps
  delay(100);
  calibrate();
  mpuAwake = true;
}

void printCfg() {
  Serial.printf("CFG,%d,%d,%d,%d,%.3f,%.2f,%.3f\n", X_AXIS, X_SIGN, Y_AXIS, Y_SIGN, SENS, DEADZONE, SMOOTH);
}

// Serial commands from the tuner:
//   C,xa,xs,ya,ys,sens,dead,smooth   set config
//   S1 / S0                          start / stop gyro stream
//   K                                recalibrate
//   P                                print config
void runCmd(char* s) {
  if (s[0] == 'C' && s[1] == ',') {
    int xa, xs, ya, ys; float se, de, sm;
    if (sscanf(s + 2, "%d,%d,%d,%d,%f,%f,%f", &xa, &xs, &ya, &ys, &se, &de, &sm) == 7) {
      X_AXIS = constrain(xa, 0, 2); X_SIGN = xs < 0 ? -1 : 1;
      Y_AXIS = constrain(ya, 0, 2); Y_SIGN = ys < 0 ? -1 : 1;
      SENS = se; DEADZONE = de; SMOOTH = constrain(sm, 0.0, 0.95);
      Serial.println("OK,C");
    }
  } else if (!strcmp(s, "S1")) { wakeMPU(); streaming = true; }
  else if (!strcmp(s, "S0")) { streaming = false; }
  else if (!strcmp(s, "K"))  { if (mpuAwake) calibrate(); }
  else if (!strcmp(s, "P"))  { printCfg(); }
}

void handleSerial() {
  static char buf[96]; static uint8_t n = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') { if (n) { buf[n] = 0; runCmd(buf); n = 0; } }
    else if (n < sizeof(buf) - 1) buf[n++] = c;
  }
}

void setup() {
  // fresh BLE address so macOS doesn't reuse stale bond keys
  uint8_t newMac[6] = {0x02, 0xA1, 0xB2, 0xC3, 0xD4, 0x10};
  esp_base_mac_addr_set(newMac);

  Serial.begin(BAUD);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.beginTransmission(MPU_ADDR);
  Serial.println(Wire.endTransmission() == 0 ? "MPU found at 0x68" : "MPU NOT FOUND");
  mpuWrite(0x6B, 0x40);  // asleep until BLE connects or tuner asks

  bleMouse.begin();
  Serial.println("Advertising as AirMouse2");
}

void loop() {
  handleSerial();

  bool conn = bleMouse.isConnected();
  if (conn && !mpuAwake) { delay(3000); wakeMPU(); Serial.println("Mouse LIVE"); }
  if (!mpuAwake) { delay(5); return; }

  static uint32_t last = 0;
  if (millis() - last < 10) return;  // 100 Hz
  last = millis();

  float g[3];
  if (!readGyro(g)) return;
  g[0] -= biasX; g[1] -= biasY; g[2] -= biasZ;

  static uint8_t tick = 0;
  if (streaming && (++tick % 2 == 0)) Serial.printf("G,%.2f,%.2f,%.2f\n", g[0], g[1], g[2]);

  if (!conn) return;

  float mx = fabs(g[X_AXIS]) > DEADZONE ? g[X_AXIS] * X_SIGN * SENS : 0;
  float my = fabs(g[Y_AXIS]) > DEADZONE ? g[Y_AXIS] * Y_SIGN * SENS : 0;
  smX = smX * SMOOTH + mx * (1 - SMOOTH);
  smY = smY * SMOOTH + my * (1 - SMOOTH);

  accX += smX; accY += smY;
  int dx = (int)accX, dy = (int)accY;
  accX -= dx; accY -= dy;
  dx = constrain(dx, -127, 127);
  dy = constrain(dy, -127, 127);
  if (dx || dy) bleMouse.move(dx, dy);

  bool l = digitalRead(BTN_LEFT) == LOW;
  bool r = digitalRead(BTN_RIGHT) == LOW;
  if (l != lastL) { l ? bleMouse.press(MOUSE_LEFT)  : bleMouse.release(MOUSE_LEFT);  lastL = l; }
  if (r != lastR) { r ? bleMouse.press(MOUSE_RIGHT) : bleMouse.release(MOUSE_RIGHT); lastR = r; }
}
