#include "Arduino_ST7789_Parallel.h"

Arduino_ST7789_Parallel::Arduino_ST7789_Parallel(
  Arduino_DataBus *bus, int8_t rst, uint8_t r, bool ips, 
  int16_t w, int16_t h,
  int16_t col_offset1, int16_t row_offset1,
  int16_t col_offset2, int16_t row_offset2)
  : Arduino_TFT(bus, rst, r, ips, w, h, col_offset1, row_offset1, col_offset2, row_offset2)
{
}

bool Arduino_ST7789_Parallel::begin(int32_t speed) {
  _override_datamode = GFX_NOT_DEFINED;
  
  return Arduino_TFT::begin(speed);
}

void Arduino_ST7789_Parallel::tftInit() {
  // Hardware reset handled by base class if rst pin is defined
  // We'll do software reset
  
  delay(100);
  
  _bus->sendCommand(0x01);  // SWRESET
  delay(120);
  
  _bus->sendCommand(0x35);  // TEON
  _bus->sendData(0x00);
  
  _bus->sendCommand(0x3A);  // COLMOD - RGB565 16-bit color
  _bus->sendData(0x05);
  
  _bus->sendCommand(0xB2);  // PORCTRL
  _bus->sendData(0x0C);
  _bus->sendData(0x0C);
  _bus->sendData(0x00);
  _bus->sendData(0x33);
  _bus->sendData(0x33);
  
  _bus->sendCommand(0xC0);  // LCMCTRL
  _bus->sendData(0x2C);
  
  _bus->sendCommand(0xC2);  // VDVVRHEN
  _bus->sendData(0x01);
  
  _bus->sendCommand(0xC3);  // VRHS
  _bus->sendData(0x12);
  
  _bus->sendCommand(0xC4);  // VDVS
  _bus->sendData(0x20);
  
  _bus->sendCommand(0xD0);  // PWCTRL1
  _bus->sendData(0xA4);
  _bus->sendData(0xA1);
  
  _bus->sendCommand(0xC6);  // FRCTRL2
  _bus->sendData(0x0F);
  
  _bus->sendCommand(0xB0);  // RAMCTRL
  _bus->sendData(0x00);
  _bus->sendData(0xC0);
  
  _bus->sendCommand(0xB7);  // GCTRL
  _bus->sendData(0x35);
  
  _bus->sendCommand(0xBB);  // VCOMS
  _bus->sendData(0x1F);
  /*
  // Gamma correction - affects color appearance
  // These values are from Pimoroni's driver for good color balance
  // To disable gamma (linear), comment out these two commands
  _bus->sendCommand(0xE0);  // PVGAMCTRL - Positive voltage gamma
  _bus->sendData(0xD0);
  _bus->sendData(0x04);
  _bus->sendData(0x0D);
  _bus->sendData(0x11);
  _bus->sendData(0x13);
  _bus->sendData(0x2B);
  _bus->sendData(0x3F);
  _bus->sendData(0x54);
  _bus->sendData(0x4C);
  _bus->sendData(0x18);
  _bus->sendData(0x0D);
  _bus->sendData(0x0B);
  _bus->sendData(0x1F);
  _bus->sendData(0x23);
  
  _bus->sendCommand(0xE1);  // NVGAMCTRL - Negative voltage gamma
  _bus->sendData(0xD0);
  _bus->sendData(0x04);
  _bus->sendData(0x0C);
  _bus->sendData(0x11);
  _bus->sendData(0x13);
  _bus->sendData(0x2C);
  _bus->sendData(0x3F);
  _bus->sendData(0x44);
  _bus->sendData(0x51);
  _bus->sendData(0x2F);
  _bus->sendData(0x1F);
  _bus->sendData(0x1F);
  _bus->sendData(0x20);
  _bus->sendData(0x23);
  */
  _bus->sendCommand(0x21);  // INVON
  _bus->sendCommand(0x11);  // SLPOUT
  delay(120);
  _bus->sendCommand(0x29);  // DISPON
  
  // Enable backlight at full brightness
  setBacklight(255);
}

void Arduino_ST7789_Parallel::writeAddrWindow(int16_t x, int16_t y, uint16_t w, uint16_t h) {
  if ((x != _currentX) || (w != _currentW)) {
    _currentX = x;
    _currentW = w;
    x += _xStart;
    _bus->writeC8D16D16(0x2A, x, x + w - 1);
  }
  
  if ((y != _currentY) || (h != _currentH)) {
    _currentY = y;
    _currentH = h;
    y += _yStart;
    _bus->writeC8D16D16(0x2B, y, y + h - 1);
  }
  
  _bus->writeCommand(0x2C);  // RAMWR
}

void Arduino_ST7789_Parallel::invertDisplay(bool i) {
  _bus->sendCommand(i ? 0x21 : 0x20);
}

void Arduino_ST7789_Parallel::displayOn() {
  _bus->sendCommand(0x29);
  setBacklight(255);  // Full brightness
}

void Arduino_ST7789_Parallel::displayOff() {
  _bus->sendCommand(0x28);
  setBacklight(0);  // Off
}

void Arduino_ST7789_Parallel::setBacklight(uint8_t brightness) {
  Arduino_PimoroniPAR8 *par_bus = (Arduino_PimoroniPAR8*)_bus;
  par_bus->setBacklight(brightness);
}
