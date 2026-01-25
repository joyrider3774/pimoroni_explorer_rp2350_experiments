#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <Arduino_GFX_Library.h>
#include "Arduino_PimoroniPAR8.h"
#include "Arduino_ST7789_Parallel.h"

// Define this to use Arduino_Canvas (framebuffer), comment out for direct drawing
#define USE_CANVAS

// COLOR macro - swaps bytes for canvas mode, normal for direct mode
#ifdef USE_CANVAS
  #define COLOR(c) ((uint16_t)(((c) >> 8) | ((c) << 8)))
#else
  #define COLOR(c) (c)
#endif

// RGB565 color definitions
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define ORANGE  0xFD20
#define PURPLE  0x780F

// I2C Addresses
#define LTR559_ADDR     0x23  // Light/Proximity
#define BME280_ADDR     0x76  // Temp/Humidity/Pressure
#define LSM6DS3_ADDR    0x6A  // IMU

// LTR-559 registers
#define LTR559_ALS_CONTR      0x80
#define LTR559_PS_CONTR       0x81
#define LTR559_ALS_MEAS_RATE  0x85
#define LTR559_ALS_DATA_CH1_0 0x88
#define LTR559_ALS_DATA_CH0_0 0x8A
#define LTR559_PS_DATA_0      0x8D

// BME280 and LSM6DS3 objects
Adafruit_BME280 bme;
Adafruit_LSM6DS3TRC lsm6ds3;

// Display objects
Arduino_PimoroniPAR8 *bus;
Arduino_ST7789_Parallel *display;

#ifdef USE_CANVAS
Arduino_Canvas *gfx;
#else
Arduino_ST7789_Parallel *gfx;
#endif

// Screen dimensions
const int16_t SCREEN_WIDTH = 320;
const int16_t SCREEN_HEIGHT = 240;

// Sensor data structure
struct SensorData {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
  float accelX, accelY, accelZ;
  float gyroX, gyroY, gyroZ;
  float lux;
  uint16_t proximity;
} sensorData;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Pimoroni Multi-Sensor Display ===");
  
  // Initialize I2C
  Wire.begin();
  delay(100);
  
  // Initialize display
  bus = new Arduino_PimoroniPAR8();
  display = new Arduino_ST7789_Parallel(bus, GFX_NOT_DEFINED, 0, false, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  if(!display->begin()) {
    Serial.println("Display init failed!");
    while(1);
  }
  
  #ifdef USE_CANVAS
  gfx = new Arduino_Canvas(SCREEN_WIDTH, SCREEN_HEIGHT, display);
  if(!gfx->begin()) {
    Serial.println("Canvas init failed!");
    while(1);
  }
  Serial.println("Display and canvas initialized!");
  #else
  gfx = display;
  Serial.println("Display initialized (direct mode)!");
  #endif
  
  display->setBacklight(255);
  
  // Show splash screen
  gfx->fillScreen(COLOR(BLACK));
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(CYAN));
  gfx->setCursor(50, 100);
  gfx->print("Initializing...");
  #ifdef USE_CANVAS
  gfx->flush();
  #endif
  
  // Initialize BME280
  if (!bme.begin(BME280_ADDR)) {
    Serial.println("Could not find BME280 sensor!");
    showError("BME280 not found!");
    while (1) delay(10);
  }
  Serial.println("BME280 initialized");
  
  // Initialize LSM6DS3TR-C
  if (!lsm6ds3.begin_I2C(LSM6DS3_ADDR)) {
    Serial.println("Could not find LSM6DS3 sensor!");
    showError("LSM6DS3 not found!");
    while (1) delay(10);
  }
  Serial.println("LSM6DS3TR-C initialized");
  
  // Initialize LTR-559
  initLTR559();
  
  Serial.println("All sensors initialized!");
  delay(500);
}

void loop() {
  static uint32_t lastUpdate = 0;
  static uint32_t frameCount = 0;
  static uint32_t fps = 0;
  static uint32_t nextFpsTime = millis() + 1000;
  
  // Read all sensors
  readBME280();
  readLSM6DS3();
  readLTR559();
  
  // Update display
  updateDisplay();
  
  #ifdef USE_CANVAS
  gfx->flush();
  #endif
  
  // Calculate FPS
  frameCount++;
  if(millis() >= nextFpsTime) {
    fps = frameCount;
    Serial.print("FPS: ");
    Serial.println(fps);
    frameCount = 0;
    nextFpsTime = millis() + 1000;
  }
  
  delay(50); // ~20 FPS update rate
}

void showError(const char* msg) {
  gfx->fillScreen(COLOR(BLACK));
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(RED));
  gfx->setCursor(10, 100);
  gfx->print("ERROR:");
  gfx->setCursor(10, 120);
  gfx->print(msg);
  #ifdef USE_CANVAS
  gfx->flush();
  #endif
}

void updateDisplay() {
  gfx->fillScreen(COLOR(BLACK));
  
  // Draw title bar
  gfx->fillRect(0, 0, SCREEN_WIDTH, 25, COLOR(BLUE));
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(10, 5);
  gfx->print("Multi-Sensor Stick");
  
  // Draw three columns
  drawEnvironmental(5, 30);
  drawMotion(110, 30);
  drawLight(220, 30);
}

void drawEnvironmental(int16_t x, int16_t y) {
  int16_t lineHeight = 20;
  
  gfx->setTextSize(1);
  
  // Section header
  gfx->setTextColor(COLOR(CYAN));
  gfx->setCursor(x, y);
  gfx->print("ENVIRON");
  
  y += lineHeight;
  
  // Temperature
  gfx->setTextColor(COLOR(RED));
  gfx->setCursor(x, y);
  gfx->print("T:");
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.temperature, 1);
  gfx->print("C");
  y += lineHeight;
  
  // Humidity
  gfx->setTextColor(COLOR(GREEN));
  gfx->setCursor(x, y);
  gfx->print("H:");
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.humidity, 1);
  gfx->print("%");
  y += lineHeight;
  
  // Pressure
  gfx->setTextColor(COLOR(BLUE));
  gfx->setCursor(x, y);
  gfx->print("P:");
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.pressure, 0);
  y += lineHeight;
  
  gfx->setCursor(x + 15, y);
  gfx->print("hPa");
  y += lineHeight;
  
  // Altitude
  gfx->setTextColor(COLOR(PURPLE));
  gfx->setCursor(x, y);
  gfx->print("A:");
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.altitude, 0);
  gfx->print("m");
  
  // Draw bar graph for temperature (0-40°C)
  y += lineHeight + 5;
  int16_t barWidth = 90;
  int16_t barHeight = 8;
  int16_t tempBar = constrain(map(sensorData.temperature * 10, 0, 400, 0, barWidth), 0, barWidth);
  gfx->drawRect(x, y, barWidth, barHeight, COLOR(RED));
  gfx->fillRect(x + 1, y + 1, tempBar - 2, barHeight - 2, COLOR(RED));
}

void drawMotion(int16_t x, int16_t y) {
  int16_t lineHeight = 20;
  
  gfx->setTextSize(1);
  
  // Section header
  gfx->setTextColor(COLOR(MAGENTA));
  gfx->setCursor(x, y);
  gfx->print("ACCEL");
  y += lineHeight;
  
  // Accelerometer
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(x, y);
  gfx->print("X:");
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.accelX, 1);
  y += lineHeight;
  
  gfx->setCursor(x, y);
  gfx->print("Y:");
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.accelY, 1);
  y += lineHeight;
  
  gfx->setCursor(x, y);
  gfx->print("Z:");
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.accelZ, 1);
  y += lineHeight + 5;
  
  // Gyroscope header
  gfx->setTextColor(COLOR(CYAN));
  gfx->setCursor(x, y);
  gfx->print("GYRO");
  y += lineHeight;
  
  // Gyroscope
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(x, y);
  gfx->print("X:");
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.gyroX, 1);
  y += lineHeight;
  
  gfx->setCursor(x, y);
  gfx->print("Y:");
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.gyroY, 1);
  y += lineHeight;
  
  gfx->setCursor(x, y);
  gfx->print("Z:");
  gfx->setCursor(x + 15, y);
  gfx->print(sensorData.gyroZ, 1);
  
  // Draw orientation indicator (simple bubble level for accel)
  y += lineHeight + 5;
  int16_t bubbleSize = 40;
  int16_t centerX = x + bubbleSize / 2;
  int16_t centerY = y + bubbleSize / 2;
  
  // Draw circle
  gfx->drawCircle(centerX, centerY, bubbleSize / 2, COLOR(WHITE));
  
  // Draw crosshair
  gfx->drawLine(centerX - bubbleSize/2, centerY, centerX + bubbleSize/2, centerY, COLOR(GREEN));
  gfx->drawLine(centerX, centerY - bubbleSize/2, centerX, centerY + bubbleSize/2, COLOR(GREEN));
  
  // Draw bubble based on X and Y acceleration
  int16_t bubbleX = centerX + constrain(sensorData.accelX * 15, -bubbleSize/2 + 3, bubbleSize/2 - 3);
  int16_t bubbleY = centerY + constrain(sensorData.accelY * 15, -bubbleSize/2 + 3, bubbleSize/2 - 3);
  gfx->fillCircle(bubbleX, bubbleY, 3, COLOR(RED));
}

void drawLight(int16_t x, int16_t y) {
  int16_t lineHeight = 20;
  
  gfx->setTextSize(1);
  
  // Section header
  gfx->setTextColor(COLOR(YELLOW));
  gfx->setCursor(x, y);
  gfx->print("LIGHT");
  y += lineHeight;
  
  // Lux
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(x, y);
  gfx->print(sensorData.lux, 1);
  y += lineHeight;
  gfx->setCursor(x, y);
  gfx->print("lux");
  y += lineHeight + 5;
  
  // Light bar (0-500 lux)
  int16_t barWidth = 90;
  int16_t barHeight = 12;
  int16_t lightBar = constrain(map(sensorData.lux * 10, 0, 2000, 0, barWidth), 0, barWidth);
  gfx->drawRect(x, y, barWidth, barHeight, COLOR(YELLOW));
  gfx->fillRect(x + 1, y + 1, lightBar - 2, barHeight - 2, COLOR(YELLOW));
  y += barHeight + 10;
  
  // Proximity header
  gfx->setTextColor(COLOR(ORANGE));
  gfx->setCursor(x, y);
  gfx->print("PROX");
  y += lineHeight;
  
  // Proximity value
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(x, y);
  gfx->print(sensorData.proximity);
  y += lineHeight;
  
  // Proximity bar (0-2047)
  int16_t proxBar = constrain(map(sensorData.proximity, 0, 2047, 0, barWidth), 0, barWidth);
  gfx->drawRect(x, y, barWidth, barHeight, COLOR(ORANGE));
  gfx->fillRect(x + 1, y + 1, proxBar - 2, barHeight - 2, COLOR(ORANGE));
  y += barHeight + 10;
  
  // Visual proximity indicator - larger circle when object is close
  int16_t indicatorSize = map(constrain(sensorData.proximity, 0, 2047), 0, 2047, 5, 35);
  gfx->drawCircle(x + 45, y + 20, indicatorSize, COLOR(ORANGE));
  if (indicatorSize > 20) {
    gfx->fillCircle(x + 45, y + 20, indicatorSize - 2, COLOR(ORANGE));
  }
}

void readBME280() {
  sensorData.temperature = bme.readTemperature();
  sensorData.humidity = bme.readHumidity();
  sensorData.pressure = bme.readPressure() / 100.0F;
  sensorData.altitude = bme.readAltitude(1013.25);
}

void readLSM6DS3() {
  sensors_event_t accel, gyro, temp;
  lsm6ds3.getEvent(&accel, &gyro, &temp);
  
  sensorData.accelX = accel.acceleration.x;
  sensorData.accelY = accel.acceleration.y;
  sensorData.accelZ = accel.acceleration.z;
  
  sensorData.gyroX = gyro.gyro.x;
  sensorData.gyroY = gyro.gyro.y;
  sensorData.gyroZ = gyro.gyro.z;
}

void initLTR559() {
  // Enable ALS: Gain 1x
  writeRegister(LTR559_ADDR, LTR559_ALS_CONTR, 0x01);
  
  // Set measurement rate: 500ms
  writeRegister(LTR559_ADDR, LTR559_ALS_MEAS_RATE, 0x03);
  
  // Enable proximity sensor
  writeRegister(LTR559_ADDR, LTR559_PS_CONTR, 0x03);
  
  delay(10);
  Serial.println("LTR-559 initialized");
}

void readLTR559() {
  // Read ALS CH0 (visible + IR)
  Wire.beginTransmission(LTR559_ADDR);
  Wire.write(LTR559_ALS_DATA_CH0_0);
  Wire.endTransmission(false);
  Wire.requestFrom(LTR559_ADDR, 2);
  uint16_t ch0 = Wire.read() | (Wire.read() << 8);
  
  // Read ALS CH1 (IR)
  Wire.beginTransmission(LTR559_ADDR);
  Wire.write(LTR559_ALS_DATA_CH1_0);
  Wire.endTransmission(false);
  Wire.requestFrom(LTR559_ADDR, 2);
  uint16_t ch1 = Wire.read() | (Wire.read() << 8);
  
  // Read proximity
  Wire.beginTransmission(LTR559_ADDR);
  Wire.write(LTR559_PS_DATA_0);
  Wire.endTransmission(false);
  Wire.requestFrom(LTR559_ADDR, 2);
  sensorData.proximity = Wire.read() | (Wire.read() << 8);
  
  // Calculate lux using the algorithm from pimoroni/ltr559-python
  float lux = 0;
  
  if (ch0 + ch1 > 0) {
    uint32_t ratio = (ch1 * 1000) / (ch0 + ch1);
    
    int ch0_c[4] = {17743, 42785, 5926, 0};
    int ch1_c[4] = {-11059, 19548, -1185, 0};
    
    int ch_idx = 3;
    if (ratio < 450) {
      ch_idx = 0;
    } else if (ratio < 640 && ratio >= 450) {
      ch_idx = 1;
    } else if (ratio < 850 && ratio >= 640) {
      ch_idx = 2;
    }
    
    lux = ((ch0 * ch0_c[ch_idx]) - (ch1 * ch1_c[ch_idx])) / 10000.0;
  }
  
  sensorData.lux = lux;
}

void writeRegister(uint8_t addr, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}
