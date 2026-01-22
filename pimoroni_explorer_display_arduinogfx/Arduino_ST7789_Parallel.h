#ifndef _ARDUINO_ST7789_PARALLEL_H_
#define _ARDUINO_ST7789_PARALLEL_H_

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "Arduino_PimoroniPAR8.h"

// Color definitions (RGB565)
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

class Arduino_ST7789_Parallel : public Arduino_TFT {
public:
  Arduino_ST7789_Parallel(Arduino_DataBus *bus, int8_t rst = GFX_NOT_DEFINED, 
                          uint8_t r = 0, bool ips = false, 
                          int16_t w = 320, int16_t h = 240,
                          int16_t col_offset1 = 0, int16_t row_offset1 = 0,
                          int16_t col_offset2 = 0, int16_t row_offset2 = 0);
  
  bool begin(int32_t speed = GFX_NOT_DEFINED) override;
  void writeAddrWindow(int16_t x, int16_t y, uint16_t w, uint16_t h) override;
  void invertDisplay(bool i) override;
  void displayOn() override;
  void displayOff() override;
  
  // Backlight control (0-255)
  void setBacklight(uint8_t brightness);

protected:
  void tftInit() override;
};

#endif // _ARDUINO_ST7789_PARALLEL_H_
