#ifndef _ARDUINO_PIMORONI_PAR8_H_
#define _ARDUINO_PIMORONI_PAR8_H_

#include <Arduino.h>
#include <Arduino_DataBus.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

// Clock speed for PIO parallel interface
// Pimoroni uses 32MHz for parallel ST7789 (reliable and tested)
#define PIO_CLOCK_HZ 32000000  // 32MHz

// Pin definitions for Pimoroni Explorer
#define EXPLORER_CS 27
#define EXPLORER_DC 28
#define EXPLORER_WR 30
#define EXPLORER_RD 31
#define EXPLORER_D0 32
#define EXPLORER_BL 26

class Arduino_PimoroniPAR8 : public Arduino_DataBus {
public:
  Arduino_PimoroniPAR8(int8_t cs = EXPLORER_CS, int8_t dc = EXPLORER_DC, 
                       int8_t wr = EXPLORER_WR, int8_t rd = EXPLORER_RD,
                       int8_t d0 = EXPLORER_D0, int8_t bl = EXPLORER_BL);
  
  bool begin(int32_t speed = GFX_NOT_DEFINED, int8_t dataMode = GFX_NOT_DEFINED) override;
  void beginWrite() override;
  void endWrite() override;
  void writeCommand(uint8_t cmd) override;
  void writeCommand16(uint16_t cmd) override;
  void write(uint8_t data) override;
  void write16(uint16_t data) override;
  void writeRepeat(uint16_t data, uint32_t len) override;
  void writeBytes(uint8_t *data, uint32_t len) override;
  void writePixels(uint16_t *data, uint32_t len) override;
  void writePattern(uint8_t *data, uint8_t len, uint32_t repeat) override;
  void writeCommandBytes(uint8_t *data, uint32_t len) override;
  
  // Backlight control (0-255)
  void setBacklight(uint8_t brightness);
  
  // Make these accessible to ST7789_Canvas
  void write_blocking_dma_public(const uint8_t *src, size_t len) {
    write_blocking_dma(src, len);
  }
  
  void wait_for_finish_public() {
    wait_for_finish();
  }
  
  int8_t get_dc_pin() { return _dc; }
  int8_t get_bl_pin() { return _bl; }

private:
  int8_t _cs, _dc, _wr, _rd, _d0, _bl;
  PIO _pio;
  uint _sm;
  uint _dma_chan;
  uint _pwm_slice;  // For backlight PWM
  dma_channel_config _dma_config;  // Store DMA config for reconfiguration
  
  void setup_pio();
  void write_blocking_dma(const uint8_t *src, size_t len);
  void wait_for_finish();
};

#endif // _ARDUINO_PIMORONI_PAR8_H_
