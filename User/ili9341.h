/// \brief ST7735 Driver modified to ILI9341 for CH32V003 by KY Lee

/// \author Li Mingjie
///  - Email:  limingjie@outlook.com
///  - GitHub: https://github.com/limingjie/
/// \date Aug 2023
/// \section References
///  - https://github.com/moononournation/Arduino_GFX
///  - https://gitee.com/morita/ch32-v003/tree/master/Driver
///  - https://github.com/cnlohr/ch32v003fun/tree/master/examples/spi_oled
/// \copyright Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0)

#ifndef __ILI9341_H__
#define __ILI9341_H__

#include "ch32v00x.h"
//#include "spi.h"
#include "ch32v00x_spi.h"
#include "font5x7.h"
#include "font7x10.h"

// Delays
#define ILI9341_RST_DELAY    50   // delay ms wait for reset finish
#define ILI9341_SLPOUT_DELAY 120  // delay ms wait for sleep out finish
#define ILI9341_TRANSPARENT	0x80000000

// System Function Command List - Write Commands Only
#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09
#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0D
//#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_SLPIN   0x10  // Sleep IN
#define ILI9341_SLPOUT  0x11  // Sleep Out
#define ILI9341_PTLON   0x12  // Partial Display Mode On
#define ILI9341_NORON   0x13  // Normal Display Mode On
#define ILI9341_INVOFF  0x20  // Display Inversion Off
#define ILI9341_INVON   0x21  // Display Inversion On
#define ILI9341_GAMSET  0x26  // Gamma Set
#define ILI9341_DISPOFF 0x28  // Display Off
#define ILI9341_DISPON  0x29  // Display On
#define ILI9341_CASET   0x2A  // Column Address Set
#define ILI9341_RASET   0x2B  // Row Address Set
#define ILI9341_RAMWR   0x2C  // Memory Write
#define ILI9341_RAMRD   0x2E

#define ILI9341_PLTAR   0x30  // Partial Area
#define ILI9341_TEOFF   0x34  // Tearing Effect Line Off
#define ILI9341_TEON    0x35  // Tearing Effect Line On
#define ILI9341_MADCTL  0x36  // Memory Data Access Control
#define ILI9341_IDMOFF  0x38  // Idle Mode Off
#define ILI9341_IDMON   0x39  // Idle Mode On
#define ILI9341_COLMOD  0x3A  // Interface Pixel Format
#define ILI9341_WRTCONT 0x3C

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDID4   0xDD

// Panel Function Command List - Only Used
#define ILI9341_GMCTRP1 0xE0  // Gamma '+' polarity Correction Characteristics Setting
#define ILI9341_GMCTRN1 0xE1  // Gamma '-' polarity Correction Characteristics Setting

// MADCTL Parameters
#define ILI9341_MADCTL_MH  0x04  // Bit 2 - Refresh Left to Right
#define ILI9341_MADCTL_RGB 0x00  // Bit 3 - RGB Order
#define ILI9341_MADCTL_BGR 0x08  // Bit 3 - BGR Order
#define ILI9341_MADCTL_ML  0x10  // Bit 4 - Scan Address Increase
#define ILI9341_MADCTL_MV  0x20  // Bit 5 - X-Y Exchange
#define ILI9341_MADCTL_MX  0x40  // Bit 6 - X-Mirror
#define ILI9341_MADCTL_MY  0x80  // Bit 7 - Y-Mirror

// COLMOD Parameter
#define ILI9341_COLMOD_16_BPP 0x05  // 101 - 16-bit/pixel

// Note: To not use CS, uncomment the following line and pull CS to ground.
//  #define ILI9341_NO_CS
#define RGB565(r, g, b) ((((r)&0xF8) << 8) | (((g)&0xFC) << 3) | ((b) >> 3))
#define BGR565(r, g, b) ((((b)&0xF8) << 8) | (((g)&0xFC) << 3) | ((r) >> 3))
#define RGB         RGB565

#define BLACK       RGB(0, 0, 0)
#define NAVY        RGB(0, 0, 123)
#define DARKGREEN   RGB(0, 125, 0)
#define DARKCYAN    RGB(0, 125, 123)
#define MAROON      RGB(123, 0, 0)
#define PURPLE      RGB(123, 0, 123)
#define OLIVE       RGB(123, 125, 0)
#define LIGHTGREY   RGB(198, 195, 198)
#define DARKGREY    RGB(123, 125, 123)
#define BLUE        RGB(0, 0, 255)
#define GREEN       RGB(0, 255, 0)
#define CYAN        RGB(0, 255, 255)
#define RED         RGB(255, 0, 0)
#define MAGENTA     RGB(255, 0, 255)
#define YELLOW      RGB(255, 255, 0)
#define WHITE       RGB(255, 255, 255)
#define ORANGE      RGB(255, 165, 0)
#define GREENYELLOW RGB(173, 255, 41)
#define PINK        RGB(255, 130, 198)

/// \brief Initialize ST7735
void tft_init(void);

/// \brief Set Cursor Position for Print Functions
/// \param x X coordinate, from left to right.
/// \param y Y coordinate, from top to bottom.
void tft_set_cursor(uint16_t x, uint16_t y);

/// \brief Set Text Color
/// \param color Text color
void tft_set_color(uint16_t color);

/// \brief Set Text Background Color
/// \param color Text background color
void tft_set_background_color(uint16_t color);

/// \brief Print a Character
/// \param c Character to print
void tft_print_char(char c);

/// \brief Print a String
/// \param str String to print
void tft_print(const char* str);

/// \brief Print an Integer
/// \param num Number to print
/// \param width Expected width of the number.
/// Align left if it is less than the width of the number.
/// Align right if it is greater than the width of the number.
void tft_print_number(int32_t num, uint16_t width);

/// \brief Draw a Pixel
/// \param x X
/// \param y Y
/// \param color Pixel color
void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/// \brief Draw a Line
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
void tft_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/// \brief Draw a Rectangle
/// \param x Start X coordinate
/// \param y Start Y coordinate
/// \param width Width
/// \param height Height
/// \param color Rectangle Color
void tft_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

/// \brief Fill a Rectangle Area
/// \param x Start X coordinate
/// \param y Start Y coordinate
/// \param width Width
/// \param height Height
/// \param color Fill Color
void tft_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

/// \brief Draw a Bitmap
/// \param x Start X coordinate
/// \param y Start Y coordinate
/// \param width Width
/// \param height Height
/// \param bitmap Bitmap
void tft_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* bitmap);

void tft_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void tft_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

#endif  // __ILI9341_H__
