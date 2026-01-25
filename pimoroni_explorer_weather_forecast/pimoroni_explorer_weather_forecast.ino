#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Arduino_GFX_Library.h>
#include "Arduino_PimoroniPAR8.h"
#include "Arduino_ST7789_Parallel.h"

// Define this to use Arduino_Canvas (framebuffer)
#define USE_CANVAS

// COLOR macro - swaps bytes for canvas mode
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
#define GRAY    0x7BEF

// Button pins - using board-defined pins from Arduino Pico
// SWITCH_A, SWITCH_B, SWITCH_X, SWITCH_Y are defined by the board

// I2C Address
#define BME280_ADDR  0x76

// Pressure history settings
#define PRESSURE_SAMPLES 12  // Track 12 readings (1 per 5 minutes = 1 hour)
#define SAMPLE_INTERVAL 300000  // 5 minutes in milliseconds

// BME280 sensor
Adafruit_BME280 bme;

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

// Time and date variables
int hours = 12;
int minutes = 0;
int seconds = 0;
int day = 1;
int month = 1;
int year = 2025;
unsigned long lastTimeUpdate = 0;

// Button states
bool btnAPressed = false;
bool btnBPressed = false;
bool btnXPressed = false;
bool btnYPressed = false;

// Time setting mode
bool settingTime = false;
enum SettingMode { SET_HOURS, SET_MINUTES, SET_DAY, SET_MONTH, SET_YEAR, SET_ALTITUDE };
SettingMode settingMode = SET_HOURS;
unsigned long lastBlink = 0;
bool blinkState = true;

// Altitude setting (meters above sea level)
int altitudeMeters = 0;  // Set to your location's altitude

// Pressure tracking for forecast
float pressureHistory[PRESSURE_SAMPLES];
int pressureIndex = 0;
unsigned long lastPressureSample = 0;
int samplesCollected = 0;

// Weather forecast types
enum Forecast {
  FORECAST_UNKNOWN,
  FORECAST_SUNNY,
  FORECAST_FAIR,
  FORECAST_CHANGING,
  FORECAST_RAIN,
  FORECAST_STORM
};

// Sensor data
struct WeatherData {
  float temperature;
  float humidity;
  float pressure;           // Local pressure
  float seaLevelPressure;   // Adjusted to sea level
  float dewPoint;
  float pressureTrend;      // hPa per hour
  Forecast forecast;
  const char* forecastText;
} weather;

// Screensaver variables
bool screensaverActive = false;
unsigned long lastActivity = 0;
const unsigned long SCREENSAVER_TIMEOUT = 300000;  // 5 minutes (300000 ms)
int clockX = 0;
int clockY = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Pimoroni Weather Clock ===");
  
  // Initialize buttons
  pinMode(SWITCH_A, INPUT_PULLUP);
  pinMode(SWITCH_B, INPUT_PULLUP);
  pinMode(SWITCH_X, INPUT_PULLUP);
  pinMode(SWITCH_Y, INPUT_PULLUP);
  
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
  Serial.println("Display initialized!");
  #endif
  
  display->setBacklight(65);  // Set to 65
  
  // Show splash
  gfx->fillScreen(COLOR(BLACK));
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(CYAN));
  gfx->setCursor(60, 100);
  gfx->print("Weather Station");
  #ifdef USE_CANVAS
  gfx->flush();
  #endif
  delay(1000);
  
  // Initialize BME280
  if (!bme.begin(BME280_ADDR)) {
    Serial.println("Could not find BME280 sensor!");
    showError("BME280 not found!");
    while (1) delay(10);
  }
  Serial.println("BME280 initialized");
  
  // Initialize pressure history
  for (int i = 0; i < PRESSURE_SAMPLES; i++) {
    pressureHistory[i] = 0;
  }
  
  // Initialize screensaver
  lastActivity = millis();
  
  delay(500);
}

void loop() {
  // Update time
  updateTime();
  
  // Handle buttons
  handleButtons();
  
  // Check for screensaver activation/deactivation
  updateScreensaver();
  
  // Read sensor and update pressure history
  readWeather();
  updatePressureHistory();
  
  // Calculate forecast
  calculateForecast();
  
  // Update display
  if (screensaverActive) {
    drawScreensaver();
  } else {
    updateDisplay();
  }
  
  #ifdef USE_CANVAS
  gfx->flush();
  #endif
  
  delay(50);
}

void updateTime() {
  unsigned long currentMillis = millis();
  
  if (!settingTime && currentMillis - lastTimeUpdate >= 1000) {
    lastTimeUpdate += 1000;  // Fixed: use += to prevent drift
    seconds++;
    
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        
        if (hours >= 24) {
          hours = 0;
          day++;
          
          // Days in month
          int daysInMonth = getDaysInMonth(month, year);
          if (day > daysInMonth) {
            day = 1;
            month++;
            
            if (month > 12) {
              month = 1;
              year++;
            }
          }
        }
      }
    }
  }
}

int getDaysInMonth(int m, int y) {
  if (m == 2) {
    // Leap year check
    if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
      return 29;
    }
    return 28;
  }
  if (m == 4 || m == 6 || m == 9 || m == 11) {
    return 30;
  }
  return 31;
}

void handleButtons() {
  bool btnA = !digitalRead(SWITCH_A);
  bool btnB = !digitalRead(SWITCH_B);
  bool btnX = !digitalRead(SWITCH_X);
  bool btnY = !digitalRead(SWITCH_Y);
  
  // Any button press resets screensaver
  if (btnA || btnB || btnX || btnY) {
    if (screensaverActive) {
      // Exit screensaver on any button press
      screensaverActive = false;
      lastActivity = millis();
      return;  // Don't process button if just exiting screensaver
    }
    lastActivity = millis();
  }
  
  // A button - Enter/exit time setting mode
  if (btnA && !btnAPressed) {
    settingTime = !settingTime;
    if (settingTime) {
      settingMode = SET_HOURS;
      Serial.println("Time/Date setting mode ENABLED");
    } else {
      Serial.println("Time/Date setting mode DISABLED");
      seconds = 0;  // Reset seconds when exiting
      lastTimeUpdate = millis();
    }
    delay(200);  // Debounce
  }
  btnAPressed = btnA;
  
  // B button - Cycle through setting modes
  if (settingTime && btnB && !btnBPressed) {
    settingMode = (SettingMode)((settingMode + 1) % 6);
    const char* modeNames[] = {"HOURS", "MINUTES", "DAY", "MONTH", "YEAR", "ALTITUDE"};
    Serial.print("Now setting: ");
    Serial.println(modeNames[settingMode]);
    delay(200);
  }
  btnBPressed = btnB;
  
  // X button - Increment
  if (settingTime && btnX && !btnXPressed) {
    switch(settingMode) {
      case SET_HOURS:
        hours++;
        if (hours >= 24) hours = 0;
        break;
      case SET_MINUTES:
        minutes++;
        if (minutes >= 60) minutes = 0;
        break;
      case SET_DAY:
        day++;
        if (day > getDaysInMonth(month, year)) day = 1;
        break;
      case SET_MONTH:
        month++;
        if (month > 12) month = 1;
        // Adjust day if needed
        if (day > getDaysInMonth(month, year)) {
          day = getDaysInMonth(month, year);
        }
        break;
      case SET_YEAR:
        year++;
        if (year > 2099) year = 2025;
        // Adjust day for leap year changes
        if (day > getDaysInMonth(month, year)) {
          day = getDaysInMonth(month, year);
        }
        break;
      case SET_ALTITUDE:
        altitudeMeters += 10;
        if (altitudeMeters > 5000) altitudeMeters = 0;
        break;
    }
    Serial.print("Date/Time: ");
    Serial.print(year);
    Serial.print("-");
    Serial.print(month);
    Serial.print("-");
    Serial.print(day);
    Serial.print(" ");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(" Altitude: ");
    Serial.print(altitudeMeters);
    Serial.println("m");
    delay(150);
  }
  btnXPressed = btnX;
  
  // Y button - Decrement
  if (settingTime && btnY && !btnYPressed) {
    switch(settingMode) {
      case SET_HOURS:
        hours--;
        if (hours < 0) hours = 23;
        break;
      case SET_MINUTES:
        minutes--;
        if (minutes < 0) minutes = 59;
        break;
      case SET_DAY:
        day--;
        if (day < 1) day = getDaysInMonth(month, year);
        break;
      case SET_MONTH:
        month--;
        if (month < 1) month = 12;
        // Adjust day if needed
        if (day > getDaysInMonth(month, year)) {
          day = getDaysInMonth(month, year);
        }
        break;
      case SET_YEAR:
        year--;
        if (year < 2025) year = 2099;
        // Adjust day for leap year changes
        if (day > getDaysInMonth(month, year)) {
          day = getDaysInMonth(month, year);
        }
        break;
      case SET_ALTITUDE:
        altitudeMeters -= 10;
        if (altitudeMeters < 0) altitudeMeters = 5000;
        break;
    }
    Serial.print("Date/Time: ");
    Serial.print(year);
    Serial.print("-");
    Serial.print(month);
    Serial.print("-");
    Serial.print(day);
    Serial.print(" ");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(" Altitude: ");
    Serial.print(altitudeMeters);
    Serial.println("m");
    delay(150);
  }
  btnYPressed = btnY;
}

void updatePressureHistory() {
  unsigned long currentMillis = millis();
  
  // Sample pressure every 5 minutes
  if (currentMillis - lastPressureSample >= SAMPLE_INTERVAL) {
    lastPressureSample = currentMillis;
    
    pressureHistory[pressureIndex] = weather.seaLevelPressure;  // Use sea level pressure
    pressureIndex = (pressureIndex + 1) % PRESSURE_SAMPLES;
    
    if (samplesCollected < PRESSURE_SAMPLES) {
      samplesCollected++;
    }
    
    Serial.print("Pressure sample #");
    Serial.print(samplesCollected);
    Serial.print(": ");
    Serial.print(weather.seaLevelPressure);
    Serial.println(" hPa (sea level)");
  }
}

void calculateForecast() {
  if (samplesCollected < 2) {
    // Not enough data yet
    weather.pressureTrend = 0;
    weather.forecast = FORECAST_UNKNOWN;
    weather.forecastText = "Collecting data...";
    return;
  }
  
  // Calculate pressure trend (change per hour)
  // Compare current pressure to oldest sample (use sea level pressure)
  int oldestIndex = (pressureIndex + PRESSURE_SAMPLES - samplesCollected) % PRESSURE_SAMPLES;
  float oldestPressure = pressureHistory[oldestIndex];
  float pressureChange = weather.seaLevelPressure - oldestPressure;
  
  // Convert to hPa per hour
  float hoursElapsed = (samplesCollected * SAMPLE_INTERVAL) / 3600000.0;
  weather.pressureTrend = pressureChange / hoursElapsed;
  
  // Forecast based on sea level pressure and trend
  // These thresholds are based on common meteorological guidelines
  
  if (weather.pressureTrend > 2.0) {
    // Rapidly rising
    weather.forecast = FORECAST_FAIR;
    weather.forecastText = "Improving";
  } else if (weather.pressureTrend > 0.5) {
    // Slowly rising
    if (weather.seaLevelPressure > 1020) {
      weather.forecast = FORECAST_SUNNY;
      weather.forecastText = "Fair Weather";
    } else {
      weather.forecast = FORECAST_FAIR;
      weather.forecastText = "Improving";
    }
  } else if (weather.pressureTrend > -0.5) {
    // Stable
    if (weather.seaLevelPressure > 1020) {
      weather.forecast = FORECAST_SUNNY;
      weather.forecastText = "Stable/Fair";
    } else if (weather.seaLevelPressure > 1000) {
      weather.forecast = FORECAST_FAIR;
      weather.forecastText = "Partly Cloudy";
    } else {
      weather.forecast = FORECAST_CHANGING;
      weather.forecastText = "Unsettled";
    }
  } else if (weather.pressureTrend > -2.0) {
    // Slowly falling
    if (weather.seaLevelPressure > 1010) {
      weather.forecast = FORECAST_CHANGING;
      weather.forecastText = "Clouding Up";
    } else {
      weather.forecast = FORECAST_RAIN;
      weather.forecastText = "Rain Likely";
    }
  } else {
    // Rapidly falling
    if (weather.seaLevelPressure < 1000) {
      weather.forecast = FORECAST_STORM;
      weather.forecastText = "Storm Warning";
    } else {
      weather.forecast = FORECAST_RAIN;
      weather.forecastText = "Rain Coming";
    }
  }
  
  Serial.print("Forecast: ");
  Serial.print(weather.forecastText);
  Serial.print(" (Trend: ");
  Serial.print(weather.pressureTrend, 2);
  Serial.println(" hPa/hr)");
}

void updateDisplay() {
  gfx->fillScreen(COLOR(BLACK));
  
  // Draw clock
  drawClock();
  
  // Draw weather info
  drawWeatherInfo();
  
  // Draw forecast
  drawForecast();
  
  // Draw button hints
  drawButtonHints();
}

void drawClock() {
  // Date display at top
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(CYAN));
  gfx->setCursor(10, 5);
  
  // Day with blink if setting
  if (settingTime && settingMode == SET_DAY) {
    if (millis() - lastBlink > 500) {
      blinkState = !blinkState;
      lastBlink = millis();
    }
    if (blinkState) {
      gfx->setTextColor(COLOR(YELLOW));
    } else {
      gfx->setTextColor(COLOR(GRAY));
    }
  } else {
    gfx->setTextColor(COLOR(CYAN));
  }
  if (day < 10) gfx->print("0");
  gfx->print(day);
  
  gfx->setTextColor(COLOR(CYAN));
  gfx->print("/");
  
  // Month with blink if setting
  if (settingTime && settingMode == SET_MONTH) {
    if (millis() - lastBlink > 500) {
      blinkState = !blinkState;
      lastBlink = millis();
    }
    if (blinkState) {
      gfx->setTextColor(COLOR(YELLOW));
    } else {
      gfx->setTextColor(COLOR(GRAY));
    }
  } else {
    gfx->setTextColor(COLOR(CYAN));
  }
  if (month < 10) gfx->print("0");
  gfx->print(month);
  
  gfx->setTextColor(COLOR(CYAN));
  gfx->print("/");
  
  // Year with blink if setting
  if (settingTime && settingMode == SET_YEAR) {
    if (millis() - lastBlink > 500) {
      blinkState = !blinkState;
      lastBlink = millis();
    }
    if (blinkState) {
      gfx->setTextColor(COLOR(YELLOW));
    } else {
      gfx->setTextColor(COLOR(GRAY));
    }
  } else {
    gfx->setTextColor(COLOR(CYAN));
  }
  gfx->print(year);
  
  // Large digital clock
  gfx->setTextSize(5);
  
  // Hours
  if (settingTime && settingMode == SET_HOURS) {
    // Blink hours when setting
    if (millis() - lastBlink > 500) {
      blinkState = !blinkState;
      lastBlink = millis();
    }
    if (blinkState) {
      gfx->setTextColor(COLOR(YELLOW));
    } else {
      gfx->setTextColor(COLOR(GRAY));
    }
  } else {
    gfx->setTextColor(COLOR(WHITE));
  }
  
  gfx->setCursor(50, 25);
  if (hours < 10) gfx->print("0");
  gfx->print(hours);
  
  // Colon
  gfx->setTextColor(COLOR(WHITE));
  gfx->print(":");
  
  // Minutes
  if (settingTime && settingMode == SET_MINUTES) {
    // Blink minutes when setting
    if (millis() - lastBlink > 500) {
      blinkState = !blinkState;
      lastBlink = millis();
    }
    if (blinkState) {
      gfx->setTextColor(COLOR(YELLOW));
    } else {
      gfx->setTextColor(COLOR(GRAY));
    }
  } else {
    gfx->setTextColor(COLOR(WHITE));
  }
  
  if (minutes < 10) gfx->print("0");
  gfx->print(minutes);
  
  // Small seconds display
  if (!settingTime) {
    gfx->setTextSize(2);
    gfx->setTextColor(COLOR(GRAY));
    gfx->setCursor(240, 45);
    if (seconds < 10) gfx->print("0");
    gfx->print(seconds);
  }
  
  // Setting mode indicator
  if (settingTime) {
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR(YELLOW));
    gfx->setCursor(70, 70);
    gfx->print("SETTING MODE");
    
    gfx->setTextColor(COLOR(CYAN));
    gfx->setCursor(50, 80);
    const char* modeNames[] = {"[HOURS]", "[MINUTES]", "[DAY]", "[MONTH]", "[YEAR]", "[ALTITUDE]"};
    gfx->print(modeNames[settingMode]);
    
    // Show altitude value when setting it
    if (settingMode == SET_ALTITUDE) {
      gfx->setTextSize(2);
      if (millis() - lastBlink > 500) {
        blinkState = !blinkState;
        lastBlink = millis();
      }
      if (blinkState) {
        gfx->setTextColor(COLOR(YELLOW));
      } else {
        gfx->setTextColor(COLOR(GRAY));
      }
      gfx->setCursor(80, 90);
      gfx->print(altitudeMeters);
      gfx->print(" m");
    }
  }
}

void drawWeatherInfo() {
  int16_t startY = 95;
  
  // Temperature - large display
  gfx->setTextSize(3);
  gfx->setTextColor(COLOR(RED));
  gfx->setCursor(10, startY);
  gfx->print(weather.temperature, 1);
  gfx->setTextSize(2);
  gfx->print("C");
  
  // Humidity
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(GREEN));
  gfx->setCursor(130, startY + 5);
  gfx->print("H:");
  gfx->setTextColor(COLOR(WHITE));
  gfx->print(weather.humidity, 0);
  gfx->print("%");
  
  startY += 35;
  
  // Pressure with trend arrow (show sea level pressure)
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(BLUE));
  gfx->setCursor(10, startY);
  gfx->print(weather.seaLevelPressure, 1);
  gfx->print(" hPa");
  
  // Trend arrow
  if (samplesCollected >= 2) {
    int16_t arrowX = 180;
    int16_t arrowY = startY + 8;
    
    if (weather.pressureTrend > 0.5) {
      // Rising - up arrow
      gfx->setTextColor(COLOR(GREEN));
      drawArrowUp(arrowX, arrowY);
    } else if (weather.pressureTrend < -0.5) {
      // Falling - down arrow
      gfx->setTextColor(COLOR(RED));
      drawArrowDown(arrowX, arrowY);
    } else {
      // Stable - horizontal line
      gfx->setTextColor(COLOR(YELLOW));
      gfx->fillRect(arrowX, arrowY + 4, 12, 2, COLOR(YELLOW));
    }
    
    // Trend value
    gfx->setTextSize(1);
    gfx->setTextColor(COLOR(GRAY));
    gfx->setCursor(200, startY + 5);
    if (weather.pressureTrend > 0) gfx->print("+");
    gfx->print(weather.pressureTrend, 1);
    gfx->print("/hr");
  }
}

void drawForecast() {
  int16_t startY = 155;
  
  // Forecast section header
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR(CYAN));
  gfx->setCursor(10, startY);
  gfx->print("FORECAST:");
  
  startY += 12;
  
  // Forecast text
  gfx->setTextSize(2);
  uint16_t forecastColor;
  
  switch(weather.forecast) {
    case FORECAST_SUNNY:
      forecastColor = COLOR(YELLOW);
      break;
    case FORECAST_FAIR:
      forecastColor = COLOR(GREEN);
      break;
    case FORECAST_CHANGING:
      forecastColor = COLOR(ORANGE);
      break;
    case FORECAST_RAIN:
      forecastColor = COLOR(BLUE);
      break;
    case FORECAST_STORM:
      forecastColor = COLOR(RED);
      break;
    default:
      forecastColor = COLOR(GRAY);
  }
  
  gfx->setTextColor(forecastColor);
  gfx->setCursor(10, startY);
  gfx->print(weather.forecastText);
  
  // Draw weather icon
  drawWeatherIcon(220, startY - 5, weather.forecast);
  
  // Additional info
  startY += 25;
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR(GRAY));
  gfx->setCursor(10, startY);
  gfx->print("Dew Point: ");
  gfx->setTextColor(COLOR(WHITE));
  gfx->print(weather.dewPoint, 1);
  gfx->print(" C");
  
  // Data collection status
  if (samplesCollected < PRESSURE_SAMPLES) {
    startY += 10;
    gfx->setTextColor(COLOR(YELLOW));
    gfx->setCursor(10, startY);
    gfx->print("Calibrating: ");
    gfx->print(samplesCollected);
    gfx->print("/");
    gfx->print(PRESSURE_SAMPLES);
    gfx->print(" samples");
  }
}

void drawWeatherIcon(int16_t x, int16_t y, Forecast forecast) {
  switch(forecast) {
    case FORECAST_SUNNY:
      // Sun
      gfx->fillCircle(x + 15, y + 15, 10, COLOR(YELLOW));
      for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4;
        int x1 = x + 15 + cos(angle) * 15;
        int y1 = y + 15 + sin(angle) * 15;
        int x2 = x + 15 + cos(angle) * 20;
        int y2 = y + 15 + sin(angle) * 20;
        gfx->drawLine(x1, y1, x2, y2, COLOR(YELLOW));
      }
      break;
      
    case FORECAST_FAIR:
      // Sun with small cloud
      gfx->fillCircle(x + 10, y + 10, 8, COLOR(YELLOW));
      gfx->fillCircle(x + 18, y + 18, 6, COLOR(WHITE));
      gfx->fillCircle(x + 25, y + 16, 7, COLOR(WHITE));
      break;
      
    case FORECAST_CHANGING:
      // Cloud
      gfx->fillCircle(x + 10, y + 15, 8, COLOR(GRAY));
      gfx->fillCircle(x + 20, y + 13, 10, COLOR(GRAY));
      gfx->fillCircle(x + 28, y + 15, 7, COLOR(GRAY));
      break;
      
    case FORECAST_RAIN:
      // Cloud with rain
      gfx->fillCircle(x + 10, y + 10, 8, COLOR(GRAY));
      gfx->fillCircle(x + 20, y + 8, 10, COLOR(GRAY));
      gfx->fillCircle(x + 28, y + 10, 7, COLOR(GRAY));
      // Rain drops
      for (int i = 0; i < 3; i++) {
        gfx->drawLine(x + 10 + i * 8, y + 22, x + 8 + i * 8, y + 28, COLOR(BLUE));
      }
      break;
      
    case FORECAST_STORM:
      // Dark cloud with lightning
      gfx->fillCircle(x + 10, y + 10, 8, COLOR(PURPLE));
      gfx->fillCircle(x + 20, y + 8, 10, COLOR(PURPLE));
      gfx->fillCircle(x + 28, y + 10, 7, COLOR(PURPLE));
      // Lightning bolt
      gfx->drawLine(x + 20, y + 20, x + 18, y + 24, COLOR(YELLOW));
      gfx->drawLine(x + 18, y + 24, x + 22, y + 24, COLOR(YELLOW));
      gfx->drawLine(x + 22, y + 24, x + 20, y + 28, COLOR(YELLOW));
      break;
      
    default:
      // Question mark
      gfx->setTextSize(2);
      gfx->setTextColor(COLOR(GRAY));
      gfx->setCursor(x + 10, y + 5);
      gfx->print("?");
  }
}

void drawArrowUp(int16_t x, int16_t y) {
  // Up arrow
  gfx->fillTriangle(x, y + 8, x + 6, y, x + 12, y + 8, COLOR(GREEN));
  gfx->fillRect(x + 4, y + 6, 4, 6, COLOR(GREEN));
}

void drawArrowDown(int16_t x, int16_t y) {
  // Down arrow
  gfx->fillTriangle(x, y, x + 6, y + 8, x + 12, y, COLOR(RED));
  gfx->fillRect(x + 4, y, 4, 6, COLOR(RED));
}

void drawButtonHints() {
  gfx->setTextSize(1);
  
  if (settingTime) {
    // Show controls when setting time/date
    gfx->setTextColor(COLOR(YELLOW));
    gfx->setCursor(5, 230);
    gfx->print("A:Exit B:Next X:+ Y:-");
  } else {
    // Show how to enter setting mode
    gfx->setTextColor(COLOR(GRAY));
    gfx->setCursor(5, 230);
    gfx->print("Press A to set time/date");
  }
}

void readWeather() {
  weather.temperature = bme.readTemperature();
  weather.humidity = bme.readHumidity();
  weather.pressure = bme.readPressure() / 100.0F;  // Local pressure
  
  // Calculate sea level equivalent pressure
  // Formula: P0 = P / (1 - (altitude / 44330))^5.255
  if (altitudeMeters > 0) {
    weather.seaLevelPressure = weather.pressure / pow(1.0 - (altitudeMeters / 44330.0), 5.255);
  } else {
    weather.seaLevelPressure = weather.pressure;
  }
  
  // Calculate dew point using Magnus formula
  float a = 17.27;
  float b = 237.7;
  float alpha = ((a * weather.temperature) / (b + weather.temperature)) + log(weather.humidity / 100.0);
  weather.dewPoint = (b * alpha) / (a - alpha);
}

void updateScreensaver() {
  unsigned long currentMillis = millis();
  
  // Don't activate screensaver while setting time
  if (settingTime) {
    lastActivity = currentMillis;
    screensaverActive = false;
    return;
  }
  
  // Check if we should activate screensaver
  if (!screensaverActive && (currentMillis - lastActivity >= SCREENSAVER_TIMEOUT)) {
    screensaverActive = true;
    Serial.println("Screensaver activated");
    // Initialize random position
    clockX = random(0, SCREEN_WIDTH - 200);
    clockY = random(40, SCREEN_HEIGHT - 100);
  }
}

void drawScreensaver() {
  // Move clock position every 10 seconds
  static unsigned long lastMove = 0;
  if (millis() - lastMove > 10000) {
    clockX = random(0, SCREEN_WIDTH - 200);
    clockY = random(40, SCREEN_HEIGHT - 100);
    lastMove = millis();
  }
  
  // Clear screen
  gfx->fillScreen(COLOR(BLACK));
  
  // Draw moving date
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(CYAN));
  gfx->setCursor(clockX, clockY - 25);
  if (day < 10) gfx->print("0");
  gfx->print(day);
  gfx->print("/");
  if (month < 10) gfx->print("0");
  gfx->print(month);
  gfx->print("/");
  gfx->print(year);
  
  // Draw moving time
  gfx->setTextSize(4);
  gfx->setTextColor(COLOR(WHITE));
  gfx->setCursor(clockX, clockY);
  if (hours < 10) gfx->print("0");
  gfx->print(hours);
  gfx->print(":");
  if (minutes < 10) gfx->print("0");
  gfx->print(minutes);
  
  // Small seconds
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(GRAY));
  gfx->setCursor(clockX + 150, clockY + 10);
  if (seconds < 10) gfx->print("0");
  gfx->print(seconds);
  
  // Temperature display
  gfx->setTextSize(2);
  gfx->setTextColor(COLOR(RED));
  gfx->setCursor(clockX, clockY + 40);
  gfx->print(weather.temperature, 1);
  gfx->print(" C");
  
  // Humidity
  gfx->setTextColor(COLOR(GREEN));
  gfx->setCursor(clockX + 120, clockY + 40);
  gfx->print(weather.humidity, 0);
  gfx->print("%");
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
