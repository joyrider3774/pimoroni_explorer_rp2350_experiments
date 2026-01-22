#include "Arduino_PimoroniPAR8.h"
#include "hardware/pwm.h"

// PIO program for parallel write
static const uint16_t st7789_parallel_program[] = {
    0x6008,  // out pins, 8  side 0
    0xb042,  // nop          side 1
};

Arduino_PimoroniPAR8::Arduino_PimoroniPAR8(int8_t cs, int8_t dc, int8_t wr, int8_t rd, int8_t d0, int8_t bl)
  : _cs(cs), _dc(dc), _wr(wr), _rd(rd), _d0(d0), _bl(bl), _pio(nullptr), _sm(0), _dma_chan(0), _pwm_slice(0)
{
}

bool Arduino_PimoroniPAR8::begin(int32_t speed, int8_t dataMode) {
  UNUSED(speed);
  UNUSED(dataMode);
  
  gpio_init(_cs);
  gpio_init(_dc);
  gpio_init(_rd);
  gpio_init(_bl);
  
  gpio_set_dir(_cs, GPIO_OUT);
  gpio_set_dir(_dc, GPIO_OUT);
  gpio_set_dir(_rd, GPIO_OUT);
  gpio_set_dir(_bl, GPIO_OUT);
  
  gpio_put(_cs, 1);
  gpio_put(_dc, 1);
  gpio_put(_rd, 1);
  
  // Setup PWM for backlight
  gpio_set_function(_bl, GPIO_FUNC_PWM);
  _pwm_slice = pwm_gpio_to_slice_num(_bl);
  pwm_set_wrap(_pwm_slice, 255);  // 8-bit resolution
  pwm_set_chan_level(_pwm_slice, pwm_gpio_to_channel(_bl), 0);  // Start at 0
  pwm_set_enabled(_pwm_slice, true);
  
  setup_pio();
  return true;
}

void Arduino_PimoroniPAR8::setup_pio() {
  _pio = pio1;
  pio_set_gpio_base(_pio, 16);
  _sm = pio_claim_unused_sm(_pio, true);
  
  // Create PIO program structure
  static pio_program_t program;
  program.instructions = st7789_parallel_program;
  program.length = 2;
  program.origin = -1;
  
  uint offset = pio_add_program(_pio, &program);
  
  // Configure SM
  pio_sm_config c = pio_get_default_sm_config();
  sm_config_set_out_pins(&c, _d0, 8);
  sm_config_set_sideset_pins(&c, _wr);
  sm_config_set_sideset(&c, 1, false, false);  // CRITICAL!
  sm_config_set_wrap(&c, offset, offset + 1);
  sm_config_set_out_shift(&c, false, true, 8);
  
  float div = (float)clock_get_hz(clk_sys) / PIO_CLOCK_HZ;
  sm_config_set_clkdiv(&c, div);
  
  Serial.print("PIO clock divider: ");
  Serial.print(div);
  Serial.print(" (target ");
  Serial.print(PIO_CLOCK_HZ / 1000000);
  Serial.println(" MHz)");
  
  pio_sm_init(_pio, _sm, offset, &c);
  
  // Init pins
  pio_gpio_init(_pio, _wr);
  for(int i = 0; i < 8; i++) {
    pio_gpio_init(_pio, _d0 + i);
  }
  
  pio_sm_set_consecutive_pindirs(_pio, _sm, _wr, 1, true);
  pio_sm_set_consecutive_pindirs(_pio, _sm, _d0, 8, true);
  
  pio_sm_set_enabled(_pio, _sm, true);
  
  // Setup DMA
  _dma_chan = dma_claim_unused_channel(true);
  _dma_config = dma_channel_get_default_config(_dma_chan);
  channel_config_set_transfer_data_size(&_dma_config, DMA_SIZE_8);
  channel_config_set_dreq(&_dma_config, pio_get_dreq(_pio, _sm, true));
  dma_channel_configure(_dma_chan, &_dma_config, &_pio->txf[_sm], NULL, 0, false);
}

void Arduino_PimoroniPAR8::write_blocking_dma(const uint8_t *src, size_t len) {
  dma_channel_set_read_addr(_dma_chan, src, false);
  dma_channel_set_trans_count(_dma_chan, len, true);
}

void Arduino_PimoroniPAR8::wait_for_finish() {
  // Wait for DMA to complete
  dma_channel_wait_for_finish_blocking(_dma_chan);
  
  // Wait for TX FIFO to drain
  while(pio_sm_get_tx_fifo_level(_pio, _sm) > 0) {
    tight_loop_contents();
  }
  
  // Wait for PIO to stall (indicating it's done processing)
  while((_pio->fdebug & (1u << (PIO_FDEBUG_TXSTALL_LSB + _sm))) == 0) {
    tight_loop_contents();
  }
  
  // Clear the stall flag
  _pio->fdebug = 1u << (PIO_FDEBUG_TXSTALL_LSB + _sm);
  
  // Extra safety - small delay
  delayMicroseconds(1);
}

void Arduino_PimoroniPAR8::beginWrite() {
  pio_sm_clear_fifos(_pio, _sm);
  gpio_put(_cs, 0);
}

void Arduino_PimoroniPAR8::endWrite() {
  wait_for_finish();
  gpio_put(_cs, 1);
}

void Arduino_PimoroniPAR8::writeCommand(uint8_t cmd) {
  pio_sm_clear_fifos(_pio, _sm);
  gpio_put(_dc, 0);  // Command mode
  wait_for_finish();
  write_blocking_dma(&cmd, 1);
  wait_for_finish();
}

void Arduino_PimoroniPAR8::writeCommand16(uint16_t cmd) {
  uint8_t c[2] = {(uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF)};
  pio_sm_clear_fifos(_pio, _sm);
  gpio_put(_dc, 0);  // Command mode
  wait_for_finish();
  write_blocking_dma(c, 2);
  wait_for_finish();
}

void Arduino_PimoroniPAR8::write(uint8_t data) {
  gpio_put(_dc, 1);  // Data mode
  write_blocking_dma(&data, 1);
  wait_for_finish();
}

void Arduino_PimoroniPAR8::write16(uint16_t data) {
  uint8_t d[2] = {(uint8_t)(data >> 8), (uint8_t)(data & 0xFF)};
  gpio_put(_dc, 1);  // Data mode
  write_blocking_dma(d, 2);
  wait_for_finish();
}

void Arduino_PimoroniPAR8::writeRepeat(uint16_t data, uint32_t len) {
  uint8_t hi = data >> 8;
  uint8_t lo = data & 0xFF;
  
  gpio_put(_dc, 1);  // Data mode
  
  // Use a small buffer and repeat it
  const size_t buf_size = 512;
  uint8_t buf[buf_size];
  
  for(size_t i = 0; i < buf_size / 2; i++) {
    buf[i * 2] = hi;
    buf[i * 2 + 1] = lo;
  }
  
  while(len >= buf_size / 2) {
    write_blocking_dma(buf, buf_size);
    wait_for_finish();
    len -= buf_size / 2;
  }
  
  if(len > 0) {
    write_blocking_dma(buf, len * 2);
    wait_for_finish();
  }
}

void Arduino_PimoroniPAR8::writeBytes(uint8_t *data, uint32_t len) {
  gpio_put(_dc, 1);  // Data mode
  
  // Send in chunks if needed
  const size_t chunk_size = 4096;
  while(len > chunk_size) {
    write_blocking_dma(data, chunk_size);
    wait_for_finish();
    data += chunk_size;
    len -= chunk_size;
  }
  
  if(len > 0) {
    write_blocking_dma(data, len);
    wait_for_finish();
  }
}

void Arduino_PimoroniPAR8::writePixels(uint16_t *data, uint32_t len) {
  gpio_put(_dc, 1);  // Data mode
  
  // Send directly - no byte swap (framebuffer is already in correct format)
  write_blocking_dma((uint8_t*)data, len * 2);
  wait_for_finish();
}

void Arduino_PimoroniPAR8::writePattern(uint8_t *data, uint8_t len, uint32_t repeat) {
  gpio_put(_dc, 1);  // Data mode
  
  while(repeat--) {
    write_blocking_dma(data, len);
    wait_for_finish();
  }
}

void Arduino_PimoroniPAR8::writeCommandBytes(uint8_t *data, uint32_t len) {
  pio_sm_clear_fifos(_pio, _sm);
  gpio_put(_dc, 0);  // Command mode
  
  wait_for_finish();
  write_blocking_dma(data, len);
  wait_for_finish();
}

void Arduino_PimoroniPAR8::setBacklight(uint8_t brightness) {
  pwm_set_chan_level(_pwm_slice, pwm_gpio_to_channel(_bl), brightness);
}
