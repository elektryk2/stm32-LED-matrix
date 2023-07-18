#include "PxMatrix.h"


// Pass 8-bit (each) R,G,B, get back 16-bit packed color
/*inline*/ uint16_t PxMATRIX::color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Init code common to both constructors
/*inline*/ void PxMATRIX::init(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A, uint8_t B){
  _LATCH_PIN = LATCH;
  _OE_PIN = OE;
  _display_color=0;

  _A_PIN = A;
  _B_PIN = B;

  _width = width;
  _height = height;

  _buffer_size = (_width*_height * 3 / 8);

  _brightness=255;
  _panels_width = 1;

  _rows_per_buffer = _height/2;
  _panel_width_bytes = (_width/_panels_width)/8;

  _active_buffer=false;

  _color_R_offset=0;
  _color_G_offset=0;
  _color_B_offset=0;

  _test_last_call=0;
  _test_pixel_counter=0;
  _test_line_counter=0;
  _rotate=0;
  _flip=0;
  _fast_update=0;

  _row_pattern=0;
  _scan_pattern=LINE;
  _block_pattern=ABCD;
  _driver_chip=SHIFT;

  _mux_delay_A=0;
  _mux_delay_B=0;
  _mux_delay_C=0;
  _mux_delay_D=0;
  _mux_delay_E=0;

    PxMATRIX_buffer= new uint8_t[PxMATRIX_COLOR_DEPTH*_buffer_size];
  #ifdef PxMATRIX_DOUBLE_BUFFER
    PxMATRIX_buffer2=new uint8_t[PxMATRIX_COLOR_DEPTH*_buffer_size];
  #endif

}

#ifdef ESP32
/*inline*/ void PxMATRIX::fm612xWriteRegister(uint16_t reg_value, uint8_t reg_position)
{
    spi_t * spi = SPI.bus();
    // reg_value = 0x1234;  debug

    for(int i=0; i<47; i++)
      SPI_2BYTE(reg_value);

    spiSimpleTransaction(spi);

    spi->dev->mosi_dlen.usr_mosi_dbitlen = 16-reg_position-1;
    spi->dev->miso_dlen.usr_miso_dbitlen = 0;
    spi->dev->data_buf[0] = reg_value>>8;
    spi->dev->cmd.usr = 1;
    while(spi->dev->cmd.usr);

    GPIO_REG_SET(1 << _LATCH_PIN);

    spi->dev->mosi_dlen.usr_mosi_dbitlen = (reg_position-8)-1;
    spi->dev->data_buf[0] = reg_value>>(reg_position-8);
    spi->dev->cmd.usr = 1;
    while(spi->dev->cmd.usr);
    spiEndTransaction(spi);

    SPI_BYTE(reg_value&0xff);

    GPIO_REG_CLEAR(1 << _LATCH_PIN);

}
#else
/*inline*/ void PxMATRIX::writeRegister(uint16_t reg_value, uint8_t reg_position)
{
  // if (_driver_chip == FM6124 || _driver_chip == FM6126A){

  //   if (_driver_chip == FM6124) {
  //      Serial.println("\nFM6124 - REG: " + String(reg_position));
  //   } else {
  //      Serial.println("\nFM6126A - REG: " + String(reg_position));
  //   }

  //   // All FM6126A code is based on the excellent guesswork by shades66 in https://github.com/hzeller/rpi-rgb-led-matrix/issues/746
  //   // Register 12 - brightness/gain settings, three 6bit values, aaaaaabbbbbbcccccc a= darkness?
  //   //               seems to add red to the background when the leds are off, b=main brightness c=finer brightness
  //   //               (i'm not sure if b & c are actually as 12 bit value but with b set to all 1's the value in c doesn't seem to make much difference)

  //   // Register 13 - not sure what it's doing yet, just that 1 specific bit within seems to be an overall enable function.

  //   // Now set all the values at the top to the same value for each of register 12/13 to get the same settings across the panel, the current code loads different settings into each 32 columns.
  //   // clocking in the register is simply clocking in the value (i've 2 panels so 128bits of data) and for the last 12/13 bits depending on the register setting the latch to high. the final drop of latch to low clocks in the configuration. this is done by sending the same value to r1/r2/g1/g2/b1/b2 at the same time to load the config into all the FM6126 chips

  //   // Some necessary magic bit fields
  //   // b12  - 1  adds red tinge
  //   // b12  - 9/8/7/6/5  =  4 bit brightness
  //   // b13  - 9   =1 screen on
  //   // b13  - 6   =1 screen off
  //   pinMode(_SPI_CLK,OUTPUT);
  //   pinMode(_SPI_MOSI,OUTPUT);
  //   digitalWrite(_SPI_CLK,HIGH); // CCK LOW
  //   digitalWrite(_OE_PIN,LOW);
  //   digitalWrite(_LATCH_PIN,HIGH);
  //   digitalWrite(_A_PIN,HIGH);
  //   digitalWrite(_B_PIN,LOW);
  //   digitalWrite(_C_PIN,LOW);
  //   digitalWrite(_D_PIN,LOW);

  //   uint8_t reg_bit=0;
  //   for (uint32_t bit_counter=0; bit_counter < _send_buffer_size*8; bit_counter++)
  //   {
  //     reg_bit=bit_counter%16;
  //     if ((reg_value>>reg_bit)&1)
  //       digitalWrite(_SPI_MOSI,HIGH);
  //     else
  //       digitalWrite(_SPI_MOSI,LOW);

  //     delay(1);
  //     digitalWrite(_SPI_CLK,LOW); // CLK HIGH
  //     delay(1);
  //     digitalWrite(_SPI_CLK,HIGH); // CLK LOW
  //     delay(1);
  //     if ((bit_counter ==  (_send_buffer_size*8 - reg_position-1)))
  //     {
  //       digitalWrite(_LATCH_PIN,LOW);
  //     }
  //   }
  //   digitalWrite(_LATCH_PIN,HIGH);
  // }
  // digitalWrite(_OE_PIN,HIGH);

}
#endif

// /*inline*/ void PxMATRIX::setDriverChip(driver_chips driver_chip)
// {
//   _driver_chip=driver_chip;

//   if (driver_chip == FM6124 || driver_chip == FM6126A){

//     uint16_t b12a=0b0111111111111111; //亮度: high
//              b12a=0b0111100011111111; //亮度: low
//  // uint16_t b12b=0b0111100000111111;
//     // uint16_t b12c=0b0111111111111111;
//     // uint16_t b12d=0b0111100000111111;

//     uint16_t b13a=0b0000000001000000;
//     // uint16_t b13b=0b0000000001000000;
//     // uint16_t b13c=0b0000000001000000;
//     // uint16_t b13d=0b0000000001000000;

// #ifdef ESP32
//     pinMode(_OE_PIN, OUTPUT);
//     pinMode(_LATCH_PIN, OUTPUT);
//     digitalWrite(_OE_PIN, HIGH);
//     pinMode(_LATCH_PIN, LOW);

//     fm612xWriteRegister(b12a,11);
//     fm612xWriteRegister(b13a,12);

// #else
//     writeRegister(b12a, 12);
//     writeRegister(b13a, 13);
// #endif


//   }
// }

// /*inline*/ void PxMATRIX::setMuxPattern(mux_patterns mux_pattern)
// {
//   _mux_pattern=mux_pattern;

//   // We handle the multiplexing in the library and activate one of for
//   // row drivers --> need A,B,C,D pins
//   if (_mux_pattern==STRAIGHT)
//   {
//     pinMode(_C_PIN, OUTPUT);
//     pinMode(_D_PIN, OUTPUT);
//   }

//   if (_mux_pattern==SHIFTREG_SPI_SE)
//   {
//     pinMode(_B_PIN, OUTPUT); // B is used as /Enable for row mux
//     digitalWrite(_B_PIN,LOW); // Enable output of row mux
//   }

//   if (_mux_pattern==SHIFTREG_ABC)
//   {
//     pinMode(_A_PIN, OUTPUT); // A is used as MUX_CLK
//     pinMode(_B_PIN, OUTPUT); // B is used as MUX_ENABLE
//     pinMode(_C_PIN, OUTPUT); // C is used as MUX_DATA
//     digitalWrite(_B_PIN,LOW); // Enable output of row mux
//   }

//   if (_mux_pattern==SHIFTREG_ABC_BIN_DE)
//   {
//     pinMode(_A_PIN, OUTPUT); // A is used as MUX_CLK
//     pinMode(_B_PIN, OUTPUT); // B is used as MUX_DATA
//     pinMode(_C_PIN, OUTPUT); // C is used as MUX_ENABLE
//     pinMode(_D_PIN, OUTPUT); // D is 4th bit of row
//     pinMode(_E_PIN, OUTPUT); // E is 5th bit of row
//     digitalWrite(_C_PIN,LOW); // Enable output of row mux
//   }
// }


/*inline*/ void PxMATRIX::setColorOrder(color_orders color_order)
{
  _color_order=color_order;
}



/*inline*/ void PxMATRIX::setMuxDelay(uint8_t mux_delay_A, uint8_t mux_delay_B, uint8_t mux_delay_C, uint8_t mux_delay_D, uint8_t mux_delay_E)
{
  _mux_delay_A=mux_delay_A;
  _mux_delay_B=mux_delay_B;
  _mux_delay_C=mux_delay_C;
  _mux_delay_D=mux_delay_D;
  _mux_delay_E=mux_delay_E;
}

/*inline*/ void PxMATRIX::setScanPattern(scan_patterns scan_pattern)
{
  _scan_pattern=scan_pattern;
}

/*inline*/ void PxMATRIX::setBlockPattern(block_patterns block_pattern)
{
  _block_pattern=block_pattern;
}

/*inline*/ void PxMATRIX::setPanelsWidth(uint8_t panels) {
  _panels_width=panels;
  _panel_width_bytes = (_width/_panels_width)/8;
}

/*inline*/ void PxMATRIX::setRotate(bool rotate) {
  _rotate=rotate;
}

/*inline*/ void PxMATRIX::setFlip(bool flip) {
  _flip=flip;
}

/*inline*/ void PxMATRIX::setFastUpdate(bool fast_update) {
  _fast_update=fast_update;
}

/*inline*/ void PxMATRIX::setBrightness(uint8_t brightness) {
  _brightness=brightness;
}


/*inline*/ PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  init(width, height, LATCH, OE, A, B);
}

/*inline*/ PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  _C_PIN = C;
  init(width, height, LATCH, OE, A, B);
}

/*inline*/ PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  _C_PIN = C;
  _D_PIN = D;
  init(width, height, LATCH, OE, A, B);
}

/*inline*/ PxMATRIX::PxMATRIX(uint16_t width, uint16_t height,uint8_t LATCH, uint8_t OE, uint8_t A,uint8_t B,uint8_t C,uint8_t D, uint8_t E) : Adafruit_GFX(width+ADAFRUIT_GFX_EXTRA, height)
{
  _C_PIN = C;
  _D_PIN = D;
  _E_PIN = E;
  init(width, height, LATCH, OE, A, B);
}

/*inline*/ void PxMATRIX::drawPixel(int16_t x, int16_t y, uint16_t color) {
  drawPixelRGB565(x, y, color);
}

/*inline*/ void PxMATRIX::showBuffer() {
  _active_buffer=!_active_buffer;
}

#ifdef PxMATRIX_DOUBLE_BUFFER
/*inline*/ void PxMATRIX::copyBuffer(bool reverse = false) {
  // This copies the display buffer to the drawing buffer (or reverse)
  // You may need this in case you rely on the framebuffer to always contain the last frame
  // _active_buffer = true means that PxMATRIX_buffer2 is displayed
  if(_active_buffer ^ reverse) {
    memcpy(PxMATRIX_buffer, PxMATRIX_buffer2, PxMATRIX_COLOR_DEPTH*_buffer_size);
  } else {
    memcpy(PxMATRIX_buffer2, PxMATRIX_buffer, PxMATRIX_COLOR_DEPTH*_buffer_size);
  }
}
#endif

/*inline*/ void PxMATRIX::setColorOffset(uint8_t r, uint8_t g,uint8_t b)
{
    _color_R_offset=r;
    _color_G_offset=g;
    _color_B_offset=b;
}

/*inline*/ void PxMATRIX::fillMatrixBuffer(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b,bool selected_buffer)
{


  if (r>_color_R_offset)
    r-=_color_R_offset;
  else
    r=0;
  if (g>_color_G_offset)
    g-=_color_G_offset;
  else
    g=0;
  if (b>_color_B_offset)
    b-=_color_B_offset;
  else
    b=0;

//  if (_block_pattern==DBCA)
//  {
//
//
//      // Every matrix is segmented in 8 blocks - 2 in X, 4 in Y direction
//      // |AB|
//      // |CD|
//      // |AB|
//      // |CD|
//      // Have to rewrite this block suff and move to the scan pattern section - this will only work for chaining up to 2 panels
//
//      if (_panels_width>1) // Only works for two panels
//      {
//        if ((x>=_width/4) && (x<_width/2))
//          x+=_width/4;
//        else if ((x>=_width/2) && (x<_width*3/4))
//          x-=_width/4;
//      }
//
//      uint16_t y_block = y*4/_height;
//      uint16_t x_block = x*2*_panels_width/_width;
//
//      // Swapping A & D
//      if (!(y_block%2)) // Even y block
//      {
//        if (!(x_block%2)) // Left side of panel
//        {
//          x+=_width/2/_panels_width;
//          y+=_height/4;
//         }
//      }
//      else // Odd y block
//      {
//        if (x_block%2) // Right side of panel
//        {
//          x-=_width/2/_panels_width;
//          y-=_height/4;
//
//        }
//      }
//
//  }

    if (_rotate){
    uint16_t temp_x=x;
    x=y;
    y=_height-1-temp_x;
  }

  // Panels are naturally flipped
  if (!_flip)
    x =_width - 1 -x;

  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
    return;

//  if (_color_order!= RRGGBB)
//  {
//    uint8_t r_temp=r;
//    uint8_t g_temp=g;
//    uint8_t b_temp=b;
//
//
//    switch (_color_order)
//    {
//      case (RRGGBB): break;
//      case (RRBBGG): g=b_temp; b=g_temp; break;
//      case (GGRRBB): r=g_temp; g=r_temp; break;
//      case (GGBBRR): r=g_temp; g=b_temp; b=r_temp; break;
//      case (BBRRGG): r=b_temp; g=r_temp; b=g_temp; break;
//      case (BBGGRR): r=b_temp; g=g_temp; b=r_temp; break;
//    }
//  }

  uint32_t base_offset;
  uint32_t total_offset_r=0;
  uint32_t total_offset_g=0;
  uint32_t total_offset_b=0;

  // if (_scan_pattern==WZAGZIG || _scan_pattern==VZAG || _scan_pattern==WZAGZIG2)
  // {
  //   // get block coordinates and constraints
  //   uint8_t rows_per_buffer = _height/2;
  //   uint8_t rows_per_block = rows_per_buffer/2;
  //   // this is a defining characteristic of WZAGZIG and VZAG:
  //   // two byte alternating chunks bottom up for WZAGZIG
  //   // two byte up down down up for VZAG
  //   uint8_t cols_per_block = 16;
  //   uint8_t panel_width = _width/_panels_width;
  //   uint8_t blocks_x_per_panel = panel_width/cols_per_block;
  //   uint8_t panel_index = x/panel_width;
  //   // strip down to single panel coordinates, restored later using panel_index
  //   x = x%panel_width;
  //   uint8_t base_y_offset = y/rows_per_buffer;
  //   uint8_t buffer_y = y%rows_per_buffer;
  //   uint8_t block_x = x/cols_per_block;
  //   uint8_t block_x_mod = x%cols_per_block;
  //   uint8_t block_y = buffer_y/rows_per_block; // can only be 0/1 for height/pattern=4
  //   uint8_t block_y_mod = buffer_y%rows_per_block;

  //   // translate block address to new block address
  //   // invert block_y so remaining translation will be more sane
  //   uint8_t block_y_inv = 1 - block_y;
  //   uint8_t block_x_inv = blocks_x_per_panel - block_x - 1;
  //   uint8_t block_linear_index;
  //   if (_scan_pattern==WZAGZIG2) {
  //     block_linear_index = block_x_inv * 2 + block_y;
  //   }
  //   else if (_scan_pattern==WZAGZIG)
  //   {
  //     // apply x/y block transform for WZAGZIG, only works for height/pattern=4
  //     block_linear_index = block_x_inv * 2 + block_y_inv;
  //   }
  //   else if (_scan_pattern==VZAG)
  //   {
  //     // apply x/y block transform for VZAG, only works for height/pattern=4 and 32x32 panels until a larger example is found
  //     block_linear_index = block_x_inv * 3 * block_y + block_y_inv  * (block_x_inv + 1);
  //   }
  //   // render block linear index back into normal coordinates
  //   uint8_t new_block_x = block_linear_index % blocks_x_per_panel;
  //   uint8_t new_block_y = 1 - block_linear_index/blocks_x_per_panel;
  //   x = new_block_x * cols_per_block + block_x_mod + panel_index * panel_width;
  //   y = new_block_y * rows_per_block + block_y_mod + base_y_offset * rows_per_buffer;
  // }

  // // This code sections computes the byte in the buffer that will be manipulated.
  // if (_scan_pattern!=LINE && _scan_pattern!=WZAGZIG && _scan_pattern!=VZAG && _scan_pattern!=WZAGZIG2)
  // {
  //   // Precomputed row offset values
  //   base_offset=_row_offset[y]-(x/8)*2;
  //   uint8_t row_sector=0;
  //   uint16_t row_sector__offset=_width/4;
  //   for (uint8_t yy = 0; yy<_height; yy+=2*_row_pattern)
  //   {
  //     if ((yy<=y) && (y<yy+_row_pattern))
  //       total_offset_r=base_offset-row_sector__offset*row_sector;
  //     if ((yy+_row_pattern<=y) && (y<yy+2*_row_pattern))
  //       total_offset_r=base_offset-row_sector__offset*row_sector;

  //     row_sector++;
  //   }
  // }
  // else
  {
    // can only be non-zero when _height/(2 inputs per panel)/_row_pattern > 1
    // i.e.: 32x32 panel with 1/8 scan (A/B/C lines) -> 32/2/8 = 2
    uint8_t vert_index_in_buffer = (y%_rows_per_buffer)/_row_pattern; // which set of rows per buffer

    // can only ever be 0/1 since there are only ever 2 separate input sets present for this variety of panels (R1G1B1/R2G2B2)
    uint8_t which_buffer = y/_rows_per_buffer;
    uint8_t x_byte = x/8;
    // assumes panels are only ever chained for more width
    uint8_t which_panel = x_byte/_panel_width_bytes;
    uint8_t in_row_byte_offset = x_byte%_panel_width_bytes;
    // this could be pretty easily extended to vertical stacking as well
    total_offset_r = _row_offset[y] - in_row_byte_offset - _panel_width_bytes*(_row_sets_per_buffer*(_panels_width*which_buffer + which_panel) + vert_index_in_buffer);
  }


  uint8_t bit_select = x%8;


  // Normally the bytes in one buffer would be sequencial, e.g.
  // 0-1-2-3-
  // 0-1-2-3-
  // hence the upper and lower row start with [OL|OH].
  //
  // However some panels have a byte wise row-changing scanning pattern and/or a bit changing pattern that we have to take case of
  // For example  [1L|1H] [3L|3H] for ZIGZAG or [0L|0H] [2L|2H] for ZAGZIG
  //                 |   \   |   \                 |   /   |   /
  //              [0L|0H] [2L|2H]               [1L|1H] [3L|3H]
  //
  // For example  [0H|1L] [2H|3L] for ZZAGG  or [0L|1L] [2L|3L] for ZZIAGG
  //                 |   \   |   \                 |   /   |   /
  //              [0L|1H] [2L|3H]               [0H|1H] [2H|3H]
  //
  //
  // In order to make the pattern start on both rows with [0L|0H] we have to add / subtract values to / from total_offset_r and bit_select
//  if ((y%(_row_pattern*2))<_row_pattern)
//  {
//    // Variant of ZAGZIG pattern with bit oder reversed on lower part (starts on upper part)
//    if (_scan_pattern==ZAGGIZ)
//    {
//        total_offset_r--;
//        bit_select = 7-bit_select;
//    }
//
//    // Row changing pattern (starts on upper part)
//    if (_scan_pattern==ZAGZIG)
//      total_offset_r--;
//
//    // Byte split pattern - like ZAGZIG but after every 4 bit (starts on upper part)
//    if (_scan_pattern == ZZIAGG )
//    {
//      if (bit_select>3)
//
//         bit_select +=4;
//      else
//        total_offset_r--;
//    }
//
//    // Byte split pattern (lower part)
//    if (_scan_pattern==ZZAGG)
//        if (bit_select>3) total_offset_r--;
//  }
//  else
//  {
//
//    if (_scan_pattern==ZIGZAG)
//      total_offset_r--;
//
//       // Byte split pattern - like ZAGZIG but after every 4 bit (starts on upper part)
//    if (_scan_pattern == ZZIAGG )
//    {
//
//      if (bit_select>3)
//      {
//        total_offset_r--;
//        bit_select -=4;
//      }
//    }
//
//    // Byte split pattern (upper part)
//    if (_scan_pattern==ZZAGG)
//    {
//      if (bit_select<=3)
//        bit_select += 4;
//      else
//      {
//        bit_select -=4;
//        total_offset_r--;
//      }
//    }
//  }

  total_offset_g=total_offset_r-_pattern_color_bytes;
  total_offset_b=total_offset_g-_pattern_color_bytes;

uint8_t *PxMATRIX_bufferp = PxMATRIX_buffer;

#ifdef PxMATRIX_DOUBLE_BUFFER
  PxMATRIX_bufferp = selected_buffer ? PxMATRIX_buffer2 : PxMATRIX_buffer;
#endif

  r = r >> (8-PxMATRIX_COLOR_DEPTH);
  g = g >> (8-PxMATRIX_COLOR_DEPTH);
  b = b >> (8-PxMATRIX_COLOR_DEPTH);

  //Color interlacing
  for (int this_color_bit=0; this_color_bit<PxMATRIX_COLOR_DEPTH; this_color_bit++)
  {
    if ((r >> this_color_bit) & 0x01)
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_r] |=_BV(bit_select);
    else
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_r] &= ~_BV(bit_select);

    if ((g >> this_color_bit) & 0x01)
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_g] |=_BV(bit_select);
    else
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_g] &= ~_BV(bit_select);

    if ((b >> this_color_bit) & 0x01)
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_b] |=_BV(bit_select);
    else
      PxMATRIX_bufferp[this_color_bit*_buffer_size+total_offset_b] &= ~_BV(bit_select);
  }
}

/*inline*/ void PxMATRIX::drawPixelRGB565(int16_t x, int16_t y, uint16_t color) {
  uint8_t r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
  uint8_t g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
  uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;
#ifdef PxMATRIX_DOUBLE_BUFFER
  fillMatrixBuffer(x, y, r, g, b, !_active_buffer);
#else
  fillMatrixBuffer(x, y, r, g, b, false);
#endif
}

/*inline*/ void PxMATRIX::drawPixelRGB888(int16_t x, int16_t y, uint8_t r, uint8_t g,uint8_t b) {
#ifdef PxMATRIX_DOUBLE_BUFFER
  fillMatrixBuffer(x, y, r, g, b, !_active_buffer);
#else
  fillMatrixBuffer(x, y, r, g, b, false);
#endif
}

// the most basic function, get a single pixel
/*inline*/ uint8_t PxMATRIX::getPixel(int8_t x, int8_t y) {
  return (0);//PxMATRIX_buffer[x+ (y/8)*LCDWIDTH] >> (y%8)) & 0x1;
}

/*inline*/ void PxMATRIX::begin()
{
  begin(8);

}

void PxMATRIX::begin(uint8_t row_pattern, uint8_t CLK, uint8_t MOSI, uint8_t MISO, uint8_t SS)
{

  _SPI_CLK = CLK;
  _SPI_MOSI = MOSI;
  _SPI_MISO = MISO;
  _SPI_SS = SS;
  begin(row_pattern);

}

void PxMATRIX::spi_init(){



}

void PxMATRIX::begin(uint8_t row_pattern) {



  _row_pattern=row_pattern;
  if (_row_pattern==4)
    _scan_pattern=ZIGZAG;
  _mux_pattern = BINARY;
  _color_order=RRGGBB;


  _pattern_color_bytes=(_height/_row_pattern)*(_width/8);
  _row_sets_per_buffer = _rows_per_buffer/_row_pattern;
  _send_buffer_size=_pattern_color_bytes*3;



  spi_init();


  //DDRE|=0b11111110;

  // pinMode(_OE_PIN, OUTPUT);
  // pinMode(_LATCH_PIN, OUTPUT);
  // pinMode(_A_PIN, OUTPUT);
  // pinMode(_B_PIN, OUTPUT);
  // digitalWrite(_A_PIN, LOW);
  // digitalWrite(_B_PIN, LOW);
  // digitalWrite(_OE_PIN, HIGH);

  // if (_row_pattern >=8)
  // {
  //   pinMode(_C_PIN, OUTPUT);
  //   digitalWrite(_C_PIN, LOW);
  // }
  // if (_row_pattern >=16)
  // {
  //   pinMode(_D_PIN, OUTPUT);
  //   digitalWrite(_D_PIN, LOW);
  // }
  // if (_row_pattern >=32)
  // {
  //   pinMode(_E_PIN, OUTPUT);
  //   digitalWrite(_E_PIN, LOW);
  // }

  // Precompute row offset values
  _row_offset=new uint32_t[_height];
  for (uint8_t yy=0; yy<_height;yy++)
      _row_offset[yy]=((yy)%_row_pattern)*_send_buffer_size+_send_buffer_size-1;

}

void PxMATRIX::set_mux(uint8_t value, bool random_access = false)
{

  if (_mux_pattern==BINARY)
  {
    if (value & 0x01) P_A_H();
    else P_A_L();
    if (_mux_delay_A) delayMicroseconds(_mux_delay_A);

    if (value & 0x02) P_B_H();
    else P_B_L();
    if (_mux_delay_B) delayMicroseconds(_mux_delay_B);

    if (_row_pattern>=8)
    {

      if (value & 0x04) P_C_H();
      else P_C_L();
      if (_mux_delay_C) delayMicroseconds(_mux_delay_C);
    }


    if (_row_pattern>=16)
    {
      if (value & 0x08) P_D_H();
      else P_D_L();
      if (_mux_delay_D) delayMicroseconds(_mux_delay_D);
    }

    // if (_row_pattern>=32)
    // {
    //   if (value & 0x10)
    //       digitalWrite(_E_PIN,HIGH);
    //   else
    //       digitalWrite(_E_PIN,LOW);
    //   if (_mux_delay_E) delayMicroseconds(_mux_delay_E);
    // }
  }

  // if (_mux_pattern==STRAIGHT)
  // {
  //   if (value==0)
  //     digitalWrite(_A_PIN,LOW);
  //   else
  //     digitalWrite(_A_PIN,HIGH);

  //   if (value==1)
  //     digitalWrite(_B_PIN,LOW);
  //   else
  //     digitalWrite(_B_PIN,HIGH);

  //   if (value==2)
  //     digitalWrite(_C_PIN,LOW);
  //   else
  //     digitalWrite(_C_PIN,HIGH);

  //   if (value==3)
  //     digitalWrite(_D_PIN,LOW);
  //   else
  //     digitalWrite(_D_PIN,HIGH);
  // }

  // if (_mux_pattern==SHIFTREG_SPI_SE) {
  //   // A, B, C on Panel are connected to a shift register Clock, /enable, Data
  //   uint8_t rowmask[4] = {0,0,0,0};
  //   if(_row_pattern > 16) {
  //     rowmask[(31-value)/8] = (1<<(value%8));
  //     SPI_TRANSFER(rowmask,4);
  //   } else {
  //     rowmask[(15-value)/8] = (1<<(value%8));
  //     SPI_TRANSFER(rowmask,2);
  //   }
  // }

  // if (_mux_pattern==SHIFTREG_ABC) {
  //   // A,B,C are connected to a shift register Clock, /Enable, Data
  //   if(random_access) {
  //     // Clock out all row mux bits to make sure random access is possible
  //     uint8_t r = _row_pattern;
  //     while(r-- > 0) {
  //       digitalWrite(_C_PIN, (value==r) ); // Shift out 1 for selected row
  //       digitalWrite(_A_PIN, HIGH); // Clock out this bit
  //       digitalWrite(_A_PIN, LOW);
  //     }
  //   } else {
  //     // Just shift the row mux by one for incremental access
  //     digitalWrite(_C_PIN, (value==0) ); // Shift out 1 for line 0, 0 otherwise
  //     digitalWrite(_A_PIN, HIGH); // Clock out this bit
  //     digitalWrite(_A_PIN, LOW);
  //   }
  // }

  // if (_mux_pattern==SHIFTREG_ABC_BIN_DE) {
  //   // A-C are connected to Shift-Register Clock, Data, /Enable, D-E to binary decoder (crazy shit)
  //   // Shift-Register is 8 rows wide, D-E select different Blocks of 8 rows
  //   digitalWrite(_D_PIN, (value>>3)&1 );
  //   digitalWrite(_E_PIN, (value>>4)&1 );
  //   value = value % 8;
  //   if(random_access) {
  //     // Clock out all row mux bits to make sure random access is possible
  //     uint8_t r = 8;
  //     while(r-- > 0) {
  //       digitalWrite(_B_PIN, (value==r) ); // Shift out 1 for selected row
  //       digitalWrite(_A_PIN, HIGH); // Clock out this bit
  //       digitalWrite(_A_PIN, LOW);
  //     }
  //   } else {
  //     // Just shift the row mux by one for incremental access
  //     digitalWrite(_B_PIN, (value==0) ); // Shift out 1 for line 0, 0 otherwise
  //     digitalWrite(_A_PIN, HIGH); // Clock out this bit
  //     digitalWrite(_A_PIN, LOW);
  //   }
  // }

}

void PxMATRIX::latch(uint16_t show_time )
{

  if (_driver_chip==SHIFT)
  {
    //digitalWrite(_OE_PIN,0); // <<< remove this
    P_LAT_H();
    //delayMicroseconds(10);
    P_LAT_L();

    //delayMicroseconds(10);
    if (show_time >0)
    {
      //delayMicroseconds(show_time);
      P_OE_L();
//      unsigned long start_time=micros();
//      while ((micros()-start_time)<show_time)
//        asm volatile (" nop ");

      delayMicroseconds(show_time);
//      delay(100);

      P_OE_H();
    }



  }

  // if (_driver_chip == FM6124 || _driver_chip==FM6126A)
  // {
  //   //digitalWrite(_OE_PIN,0); // <<< remove this
  //   digitalWrite(_LATCH_PIN,LOW);
  //   digitalWrite(_SPI_CLK,LOW);
  //   for (uint8_t latch_count=0; latch_count<3; latch_count++)
  //   {
  //     digitalWrite(_SPI_CLK,HIGH);
  //     delayMicroseconds(1);
  //     digitalWrite(_SPI_CLK,LOW);
  //     delayMicroseconds(1);
  //   }
  //   digitalWrite(_LATCH_PIN,HIGH);
  //   digitalWrite(_OE_PIN,0); //<<<< insert this
  //   delayMicroseconds(show_time);
  //   digitalWrite(_OE_PIN,1);
  // }
}

 void PxMATRIX::display()
{
  display(PxMATRIX_DEFAULT_SHOWTIME);
}

void PxMATRIX::display(uint16_t show_time) {
  if (show_time == 0)
    show_time =1;

  // How long do we keep the pixels on
  uint16_t latch_time = ((show_time*(1<<_display_color)*_brightness)/255/2);

  unsigned long start_time=0;
#ifdef ESP8266
  ESP.wdtFeed();
#endif

uint8_t *PxMATRIX_bufferp = PxMATRIX_buffer;

#ifdef PxMATRIX_DOUBLE_BUFFER
  PxMATRIX_bufferp = _active_buffer ? PxMATRIX_buffer2 : PxMATRIX_buffer;
#endif

  for (uint8_t i=0;i<_row_pattern;i++)
  {
    if(_driver_chip == SHIFT) {
       if ((_fast_update)&&(_brightness==255)){

         // This will clock data into the display while the outputs are still
         // latched (LEDs on). We therefore utilize SPI transfer latency as LED
         // ON time and can reduce the waiting time (show_time). This is rather
         // timing sensitive and may lead to flicker however promises reduced
         // update times and increased brightness

         set_mux(i);

         P_LAT_H();
         P_LAT_L();
         P_OE_L();

         start_time = micros();
         delayMicroseconds(1);
         if (i<_row_pattern-1)
         {
           // This pre-buffers the data for the next row pattern of this _display_color
             SPI_TRANSFER(&PxMATRIX_bufferp[_display_color*_buffer_size+(i+1)*_send_buffer_size],_send_buffer_size);
             memset(&PxMATRIX_bufferp[_display_color*_buffer_size+(i+1)*_send_buffer_size],0,_send_buffer_size);

         }
         else
         {
           // This pre-buffers the data for the first row pattern of the next _display_color
        	 SPI_TRANSFER(&PxMATRIX_bufferp[((_display_color+1)%PxMATRIX_COLOR_DEPTH)*_buffer_size],_send_buffer_size);

        	 memset(&PxMATRIX_bufferp[((_display_color+1)%PxMATRIX_COLOR_DEPTH)*_buffer_size], 0,_send_buffer_size);
         }

         while ((micros()-start_time)<latch_time)
          delayMicroseconds(1);
        	// asm volatile (" nop ");
         P_OE_H();

       }
       else
      {
        set_mux(i);
//#ifdef __AVR__
//  uint8_t this_byte;
//  for (uint32_t byte_cnt=0; byte_cnt<_send_buffer_size;byte_cnt++)
//  {
//    this_byte=PxMATRIX_bufferp[_display_color*_buffer_size+i*_send_buffer_size+byte_cnt];
//    SPI_BYTE(this_byte);
//  }
//#else
//  SPI_TRANSFER(&PxMATRIX_bufferp[_display_color*_buffer_size+i*_send_buffer_size],_send_buffer_size);
//#endif

  SPI_TRANSFER(&PxMATRIX_bufferp[_display_color*_buffer_size+i*_send_buffer_size],_send_buffer_size);
        latch(latch_time);
      }
    }

    // if (_driver_chip == FM6124 || _driver_chip == FM6126A) // _driver_chip == FM6124
    // {
    // #ifdef ESP32

    //   GPIO_REG_CLEAR( 1 << _OE_PIN);
    //   uint8_t* bf = &PxMATRIX_bufferp[_display_color*_buffer_size+i*_send_buffer_size];

    //   spi_t * spi = SPI.bus();
    //   spiSimpleTransaction(spi);

    //   spiWriteNL(spi, bf, _send_buffer_size-1);
    //   uint8_t v = bf[_send_buffer_size - 1];

    //   GPIO_REG_SET( 1 << _OE_PIN);

    //   spi->dev->mosi_dlen.usr_mosi_dbitlen = 4;
    //   spi->dev->miso_dlen.usr_miso_dbitlen = 0;
    //   spi->dev->data_buf[0] = v;
    //   spi->dev->cmd.usr = 1;
    //   while(spi->dev->cmd.usr);

    //   GPIO_REG_SET( 1 << _LATCH_PIN);

    //   spi->dev->mosi_dlen.usr_mosi_dbitlen = 2;
    //   spi->dev->data_buf[0] = v<<5;
    //   spi->dev->cmd.usr = 1;
    //   while(spi->dev->cmd.usr);
    //   GPIO_REG_CLEAR( 1 << _LATCH_PIN);

    //   spiEndTransaction(spi);
    //   set_mux(i);
    // #else
    //   #if defined(ESP8266) || defined(ESP32)
    //     pinMode(_SPI_CLK, SPECIAL);
    //     pinMode(_SPI_MOSI, SPECIAL);
    //   #endif
    //   SPI_TRANSFER(&PxMATRIX_bufferp[_display_color*_buffer_size+i*_send_buffer_size],_send_buffer_size-1);
    //   pinMode(_SPI_CLK, OUTPUT);
    //   pinMode(_SPI_MOSI, OUTPUT);
    //   pinMode(_SPI_MISO, OUTPUT);
    //   pinMode(_SPI_SS, OUTPUT);
    //   set_mux(i);

    //   uint8_t v = PxMATRIX_bufferp[_display_color*_buffer_size+i*_send_buffer_size + _send_buffer_size - 1];
    //   for (uint8_t this_byte = 0; this_byte < 8; this_byte++) {
    //     if (((v >> (7 - this_byte)) & 1))
    //       GPIO_REG_SET( 1 << _SPI_MOSI);
    //     else
    //       GPIO_REG_CLEAR( 1 << _SPI_MOSI);
    //     GPIO_REG_SET( 1 << _SPI_CLK);
    //     GPIO_REG_CLEAR( 1 << _SPI_CLK);

    //     if (this_byte == 4)
    //       //GPIO_REG_SET( 1 << _LATCH_PIN);
    //       digitalWrite(_LATCH_PIN, HIGH);
    //   }
    //   //GPIO_REG_WRITE(GPIO_  spi_init();

    //   digitalWrite(_LATCH_PIN, LOW);
    //   //GPIO_REG_SET( 1 << _OE_PIN);
    //   digitalWrite(_OE_PIN, 0); //<<<< insert this
    //   unsigned long start_time = micros();

    //   while ((micros()-start_time)<latch_time)
    //     delayMicroseconds(1);
    //   //GPIO_REG_CLEAR( 1 << _OE_PIN);
    //   digitalWrite(_OE_PIN, 1);
    //   //latch(show_time*(uint16_t)_brightness/255);
    // #endif
    // }
  }
  _display_color++;
  if (_display_color>=PxMATRIX_COLOR_DEPTH)
  {
    _display_color=0;
  }
}

void PxMATRIX::flushDisplay(void) {
  for (int ii=0;ii<_send_buffer_size;ii++)
    SPI_BYTE(0);
}

// void PxMATRIX::displayTestPattern(uint16_t show_time) {

//   if ((millis()-_test_last_call)>500)
//   {
//     flushDisplay();
//     for (int ii=0;ii<=_test_pixel_counter;ii++)
//       SPI_BYTE(0xFF);
//     _test_last_call=millis();
//     _test_pixel_counter++;
//   }

//   if (_test_pixel_counter>_send_buffer_size)

//   {
//     _test_pixel_counter=0;
//     _test_line_counter++;
//     flushDisplay();
//   }

//   if (_test_line_counter> (_height/2))
//         _test_line_counter=0;

//   digitalWrite(_A_PIN,HIGH);
//   delayMicroseconds(1);
//   digitalWrite(_B_PIN,HIGH);
//   delayMicroseconds(1);
//   digitalWrite(_C_PIN,HIGH);
//   delayMicroseconds(1);
//   digitalWrite(_D_PIN,HIGH);
//   delayMicroseconds(1);
//   digitalWrite(_E_PIN,HIGH);
//   delayMicroseconds(1);

//   digitalWrite(_A_PIN,LOW);
//   delayMicroseconds(1);
//   digitalWrite(_B_PIN,LOW);
//   delayMicroseconds(1);
//   digitalWrite(_C_PIN,LOW);
//   delayMicroseconds(1);
//   digitalWrite(_D_PIN,LOW);
//   delayMicroseconds(1);
//   digitalWrite(_E_PIN,LOW);
//   delayMicroseconds(1);
//   set_mux(_test_line_counter, true);

//   latch(show_time);
// }

// void PxMATRIX::displayTestPixel(uint16_t show_time) {
//   if ((millis()-_test_last_call)>500)
//   {
//     flushDisplay();
//     uint16_t blanks = _test_pixel_counter/8;
//     SPI_BYTE(1<<_test_pixel_counter%8);
//     while (blanks){
//       SPI_BYTE(0x00);
//       blanks--;
//     }
//     _test_last_call=millis();
//     _test_pixel_counter++;
//   }

//   if (_test_pixel_counter>_send_buffer_size/3*8)

//   {
//     _test_pixel_counter=0;
//     _test_line_counter++;
//   }

//   if (_test_line_counter> (_height/2))
//         _test_line_counter=0;

//   digitalWrite(_A_PIN,HIGH);
//   delayMicroseconds(1);
//   digitalWrite(_B_PIN,HIGH);
//   delayMicroseconds(1);
//   digitalWrite(_C_PIN,HIGH);
//   delayMicroseconds(1);
//   digitalWrite(_D_PIN,HIGH);
//   delayMicroseconds(1);
//   digitalWrite(_E_PIN,HIGH);
//   delayMicroseconds(1);

//   digitalWrite(_A_PIN,LOW);
//   delayMicroseconds(1);
//   digitalWrite(_B_PIN,LOW);
//   delayMicroseconds(1);
//   digitalWrite(_C_PIN,LOW);
//   delayMicroseconds(1);
//   digitalWrite(_D_PIN,LOW);
//   delayMicroseconds(1);
//   digitalWrite(_E_PIN,LOW);
//   delayMicroseconds(1);

//   set_mux(_test_line_counter, true);

//   latch(show_time);
// }


void PxMATRIX::writeText(char const * text)
{
	while(*text)
		write(*text++);
}

void PxMATRIX::clearDisplay(void) {
#ifdef PxMATRIX_DOUBLE_BUFFER
  clearDisplay(!_active_buffer);
#else
  clearDisplay(false);
#endif
}
//    void * memset ( void * ptr, int value, size_t num );

// clear everything
void PxMATRIX::clearDisplay(bool selected_buffer) {
#ifdef PxMATRIX_DOUBLE_BUFFER
  if(selected_buffer)
    memset(PxMATRIX_buffer2, 0, PxMATRIX_COLOR_DEPTH*_buffer_size);
  else
    memset(PxMATRIX_buffer, 0, PxMATRIX_COLOR_DEPTH*_buffer_size);
#else
    memset(PxMATRIX_buffer, 0, PxMATRIX_COLOR_DEPTH*_buffer_size);
#endif
}


