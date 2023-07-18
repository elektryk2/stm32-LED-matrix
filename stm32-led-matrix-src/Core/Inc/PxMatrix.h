/*********************************************************************
This is a library for Chinese LED matrix displays

Written by Dominic Buchstaller.
BSD license, check license.txt for more information
*********************************************************************/


#ifndef _PxMATRIX_H
#define _PxMATRIX_H

#include "main.h"
#include "fun.h"
#include <string.h>
#define __AVR__

#define P_A_H() GPIOB->ODR|= (1<<10)
#define P_A_L() GPIOB->ODR&=~(1<<10)

#define P_B_H() GPIOB->ODR|= (1<<11)
#define P_B_L() GPIOB->ODR&=~(1<<11)

#define P_C_H() GPIOB->ODR|= (1<<12)
#define P_C_L() GPIOB->ODR&=~(1<<12)

#define P_D_H() GPIOB->ODR|= (1<<13)
#define P_D_L() GPIOB->ODR&=~(1<<13)

///#define P_E_H() PORTE|= (1<<7)
//#define P_E_L() PORTE&=~(1<<7)

#define P_LAT_H() GPIOB->ODR|= (1<<14)
#define P_LAT_L() GPIOB->ODR&=~(1<<14)

#define P_OE_H() GPIOB->ODR|= (1<<15)
#define P_OE_L() GPIOB->ODR&=~(1<<15)

//#define PxMATRIX_DOUBLE_BUFFER 1
#define double_buffer

// Color depth per primary color - the more the slower the update
#ifndef PxMATRIX_COLOR_DEPTH
#define PxMATRIX_COLOR_DEPTH 4
#endif

#if PxMATRIX_COLOR_DEPTH > 8 || PxMATRIX_COLOR_DEPTH < 1
#error "PxMATRIX_COLOR_DEPTH must be 1 to 8 bits maximum"
#endif

// Defines how long we display things by default
#ifndef PxMATRIX_DEFAULT_SHOWTIME
#define PxMATRIX_DEFAULT_SHOWTIME 30
#endif

// Defines the speed of the SPI bus (reducing this may help if you experience noisy images)
#ifndef PxMATRIX_SPI_FREQUENCY
#define PxMATRIX_SPI_FREQUENCY 20000000
#endif

// Legacy suppport
#ifdef double_buffer
#define PxMATRIX_DOUBLE_BUFFER true
#endif

#ifndef _BV
#define _BV(x) (1 << (x))
#endif

#if defined(ESP8266) || defined(ESP32)
  #define SPI_TRANSFER(x,y) SPI.writeBytes(x,y)
  #define SPI_BYTE(x) SPI.write(x)
  #define SPI_2BYTE(x) SPI.write16(x)
#endif

#ifdef __AVR__
  #define SPI_TRANSFER(x,y) HAL_SPI_Transmit(&hspi1, x,y, HAL_MAX_DELAY) //SPI.transfer(x,y)
  //#define SPI_BYTE(x) uint8_t z=x; HAL_SPI_Transmit(&hspi1, &z,1, HAL_MAX_DELAY)//SPI.transfer(x)
inline void SPI_BYTE(uint8_t x)
{
	HAL_SPI_Transmit(&hspi1, &x,1, HAL_MAX_DELAY);
}
//  #define SPI_2BYTE(x) HAL_SPI_Transmit(&hspi1, (uint16_t*)&x,2, HAL_MAX_DELAY)//SPI.transfer16(x)
#endif

#include "Adafruit_GFX.h"
//#include "Arduino.h"
//#include <SPI.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
//#include "WProgram.h"
#endif

#ifdef __AVR__
//#include <util/delay.h>
#endif



#include <stdlib.h>

// Sometimes some extra width needs to be passed to Adafruit GFX constructor
// to render text close to the end of the display correctly
#ifndef ADAFRUIT_GFX_EXTRA
#define ADAFRUIT_GFX_EXTRA 0
#endif



#ifdef ESP8266
  #define GPIO_REG_SET(val) GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS,val)
  #define GPIO_REG_CLEAR(val) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS,val)
#endif
#ifdef ESP32
  #define GPIO_REG_SET(val) GPIO.out_w1ts = val
  #define GPIO_REG_CLEAR(val) GPIO.out_w1tc = val
#endif
#ifdef __AVR__
  #define GPIO_REG_SET(val) (val < 8) ? PORTD |= _BV(val) : PORTB |= _BV(val-8)
  #define GPIO_REG_CLEAR(val) (val < 8) ? PORTD &= ~_BV(val) : PORTB &= ~_BV(val-8)
#endif

#ifdef ESP32

  #include "soc/spi_struct.h"
  #include "esp32-hal-gpio.h"

  struct spi_struct_t {
      spi_dev_t * dev;
  #if !CONFIG_DISABLE_HAL_LOCKS
      xSemaphoreHandle lock;
  #endif
      uint8_t num;
  };
#endif

// HW SPI PINS
#define SPI_BUS_CLK 11
#define SPI_BUS_MOSI 12
#define SPI_BUS_MISO 13
#define SPI_BUS_SS 10

// Specify how the Panel handles row muxing:
// BINARY: Pins A-E map to rows 1-32 via binary decoding (default)
// STRAIGHT: Pins A-D are directly mapped to rows 1-4
// SHIFTREG: A, B, C on Panel are connected to a shift register Clock, /Enable, Data
// SHIFTREG_ABC_BIN_DE: A-C are connected to Shift-Register Clock, Data, /Enable, D-E to binary decoder (crazy shit)
// SHIFTREG_SPI_SE: Like SHIFTREG, but you connect A and C on Panel to its Clock and Data output (and ground B). This will not work with fast_update enabled!
enum mux_patterns {BINARY, STRAIGHT, SHIFTREG_ABC, SHIFTREG_SPI_SE, SHIFTREG_ABC_BIN_DE};

// Specifies what blocking pattern the panel is using 
// |AB|,|DB|
// |CD|,|CA|
// |AB|,|DB|
// |CD|,|CA|
enum block_patterns {ABCD, DBCA};

// This is how the scanning is implemented. LINE just scans it left to right,
// ZIGZAG jumps 4 rows after every byte, ZAGGII alse revereses every second byte
enum scan_patterns {LINE, ZIGZAG,ZZAGG, ZAGGIZ, WZAGZIG, VZAG, ZAGZIG, WZAGZIG2, ZZIAGG};

// Specifies speciffic driver chip. Most panels implement a standard shifted
// register (SHIFT). Other chips/panels may need special treatment in oder to work
enum driver_chips {SHIFT, FM6124, FM6126A};

// Specify the color order
enum color_orders {RRGGBB, RRBBGG, GGRRBB, GGBBRR, BBRRGG, BBGGRR};


#define color_step (256 / PxMATRIX_COLOR_DEPTH)
#define color_half_step (int(color_step / 2))
#define color_third_step (int(color_step / 3))
#define color_two_third_step (int(color_third_step*2))

class PxMATRIX : public Adafruit_GFX
{
 public:
  /*inline*/ PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B);
  /*inline*/ PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C);
  /*inline*/ PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D);
  /*inline*/ PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D,uint8_t E);

  /*inline*/ void begin(uint8_t row_pattern, uint8_t CLK, uint8_t MOSI, uint8_t MISO, uint8_t SS);
  /*inline*/ void begin(uint8_t row_pattern);
  /*inline*/ void begin();

  /*inline*/ void clearDisplay(void);
  /*inline*/ void clearDisplay(bool selected_buffer);

  // Updates the display
  /*inline*/ void display(uint16_t show_time);
  /*inline*/ void display();

  // Draw pixels
  /*inline*/ void drawPixelRGB565(int16_t x, int16_t y, uint16_t color);

  /*inline*/ void drawPixel(int16_t x, int16_t y, uint16_t color);

  /*inline*/ void drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g,uint8_t b);

  // Does nothing for now (always returns 0)
  uint8_t getPixel(int8_t x, int8_t y);

  // Converts RGB888 to RGB565
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

  // Helpful for debugging (place in display update loop)
  /*inline*/ void displayTestPattern(uint16_t showtime);

  // Helpful for debugging (place in display update loop)
  /*inline*/ void displayTestPixel(uint16_t show_time);

  // FLush the buffer of the display
  /*inline*/ void flushDisplay();

  // Rotate display
  /*inline*/ void setRotate(bool rotate);

  // Flip display
  /*inline*/ void setFlip(bool flip);

  // Helps to reduce display update latency on larger displays
  /*inline*/ void setFastUpdate(bool fast_update);

  // When using double buffering, this displays the draw buffer
  /*inline*/ void showBuffer();

#ifdef PxMATRIX_DOUBLE_BUFFER
  // This copies the display buffer to the drawing buffer (or reverse)
  /*inline*/ void copyBuffer(bool reverse);
#endif

  // Control the minimum color values that result in an active pixel
  /*inline*/ void setColorOffset(uint8_t r, uint8_t g,uint8_t b);

  // Set the multiplex implemention {BINARY, STRAIGHT, SHIFTREG} (default is BINARY)
  /*inline*/ void setMuxPattern(mux_patterns mux_pattern);

    // Set the color order
  /*inline*/ void setColorOrder(color_orders color_order);

  // Set the time in microseconds that we pause after selecting each mux channel
  // (May help if some rows are missing / the mux chip is too slow)
  /*inline*/ void setMuxDelay(uint8_t mux_delay_A, uint8_t mux_delay_B, uint8_t mux_delay_C, uint8_t mux_delay_D, uint8_t mux_delay_E);

  // Set the multiplex pattern {LINE, ZIGZAG, ZAGGIZ, WZAGZIG, VZAG, WZAGZIG2} (default is LINE)
  /*inline*/ void setScanPattern(scan_patterns scan_pattern);

 // Set the block pattern {ABCD, DBCA} (default is ABCD)
  /*inline*/ void setBlockPattern(block_patterns block_pattern);

  // Set the number of panels that make up the display area width (default is 1)
  /*inline*/ void setPanelsWidth(uint8_t panels);

  // Set the brightness of the panels (default is 255)
  /*inline*/ void setBrightness(uint8_t brightness);

  // Set driver chip type
  /*inline*/ void setDriverChip(driver_chips driver_chip);

 private:

 // the display buffer for the LED matrix
  uint8_t *PxMATRIX_buffer;
#ifdef PxMATRIX_DOUBLE_BUFFER
  uint8_t *PxMATRIX_buffer2;
#endif

  // GPIO pins
  uint8_t _LATCH_PIN;
  uint8_t _OE_PIN;
  uint8_t _A_PIN;
  uint8_t _B_PIN;
  uint8_t _C_PIN;
  uint8_t _D_PIN;
  uint8_t _E_PIN;

  // SPI pins
  uint8_t _SPI_CLK = SPI_BUS_CLK;
  uint8_t _SPI_MOSI = SPI_BUS_MOSI;
  uint8_t _SPI_MISO = SPI_BUS_MISO;
  uint8_t _SPI_SS = SPI_BUS_SS;

  uint16_t _width;
  uint16_t _height;
  uint8_t _panels_width;
  uint8_t _rows_per_buffer;
  uint8_t _row_sets_per_buffer;
  uint8_t _panel_width_bytes;

  // Color offset
  uint8_t _color_R_offset;
  uint8_t _color_G_offset;
  uint8_t _color_B_offset;

  // Panel Brightness
  uint8_t _brightness;

  // Color pattern that is pushed to the display
  uint8_t _display_color;

  // Holds some pre-computed values for faster pixel drawing
  uint32_t *_row_offset;

  // Holds the display row pattern type
  uint8_t _row_pattern;

  // Number of bytes in one color
  uint8_t _pattern_color_bytes;

  // Total number of bytes that is pushed to the display at a time
  // 3 * _pattern_color_bytes
  uint16_t _buffer_size;
  uint16_t _send_buffer_size;

  // This is for double buffering
  bool _active_buffer;

  // Display and color engine
  bool _rotate;
  bool _flip;
  bool _fast_update;

  // Holds multiplex pattern
  mux_patterns _mux_pattern;

  //Holdes the color order
  color_orders _color_order;

  uint8_t _mux_delay_A;
  uint8_t _mux_delay_B;
  uint8_t _mux_delay_C;
  uint8_t _mux_delay_D;
  uint8_t _mux_delay_E;

  // Holds the scan pattern
  scan_patterns _scan_pattern;

  // Holds the block pattern
  block_patterns _block_pattern;

  // Holds the used driver chip
  driver_chips _driver_chip;

  // Used for test pattern
  uint16_t _test_pixel_counter;
  uint16_t _test_line_counter;
  unsigned long _test_last_call;

  // Generic function that draw one pixel
/*inline*/ void fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, uint8_t g,uint8_t b, bool selected_buffer);

  // Init code common to both constructors
/*inline*/ void init(uint16_t width, uint16_t height ,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B);

  // Light up LEDs and hold for show_time microseconds
/*inline*/ void latch(uint16_t show_time );

  // Set row multiplexer
/*inline*/ void set_mux(uint8_t value, bool random_access);

/*inline*/ void spi_init();

// Write configuration register in some driver chips
/*inline*/ void writeRegister(uint16_t reg_value, uint8_t reg_position);
/*inline*/ void fm612xWriteRegister(uint16_t reg_value, uint8_t reg_position);
public:
void writeText(char const * text);

};














#endif
