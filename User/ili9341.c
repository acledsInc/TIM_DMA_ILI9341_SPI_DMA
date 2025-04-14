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

#include "ch32v00x.h"
#include <debug.h>
#include <stdint.h>

#include "ch32v00x_spi.h"
#include "ili9341.h"

#include "font5x7.h"
#define FONT_WIDTH  5  // Font width
#define FONT_HEIGHT 7  // Font height

//#include "fonts.h"
#define ILI9341_X_OFFSET 0
#define ILI9341_Y_OFFSET 0

// CH32V003 Pin Definitions
//#define SPI_RESET 0  // PC0 // not used
#define SPI_DC    GPIO_Pin_3  // PC3 =PP
#define SPI_CS    GPIO_Pin_4  // PC4 =PP
#define SPI_SCLK  GPIO_Pin_5  // PC5 =AF
#define SPI_MOSI  GPIO_Pin_6  // PC6 =AF
#define SPI_MISO  GPIO_Pin_7  // PC7 =FL (NOT USED)

// Use SPI_INIT
#define CTLR1_SPE_Set      ((uint16_t)0x0040)
#define GPIO_CNF_OUT_PP    0x00
#define GPIO_CNF_OUT_PP_AF 0x08

#define SPI_DATA_8B()			(SPI1->CTLR1 &= ~SPI_CTLR1_DFF)
#define SPI_DATA_16B()			(SPI1->CTLR1 |= SPI_CTLR1_DFF)
#define SPI_DMA_MEM_INC_ON()	(DMA1_Channel3->CFGR |= DMA_CFGR1_MINC)
#define SPI_DMA_MEM_INC_OFF()	(DMA1_Channel3->CFGR &= ~DMA_CFGR1_MINC)

typedef enum 
{
	ili9341_landscape = 0,
	ili9341_portrait
} ili9341_orient_mode_t;

typedef struct 
{
	uint16_t width;
	uint16_t height;
	ili9341_orient_mode_t lcd_orientation;
} ili9341_t;

uint16_t ili9341_x;
uint16_t ili9341_y;
ili9341_t ILI9341;

static u16 _cursor_x =0;
static u16 _cursor_y =0;        // Cursor position (x, y)
static u16 _color  =WHITE;      // Color
static u16 _bg_color =BLACK;    // Background color
static u8  _buffer[128 << 1] = {0}; // DMA buffer, long enough to fill a row.

// brief Initialize ST7735
// details Configure SPI, DMA, and RESET/DC/CS lines.
static void SPI_init()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef  SPI_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // DC =PC3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // CS =PC4
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // HW SPI1, SCLK =PC5, MOSI =PC6
    GPIO_InitStructure.GPIO_Pin = SPI_SCLK | SPI_MOSI;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // HW MISO =PC7
    GPIO_InitStructure.GPIO_Pin = SPI_MISO;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // Configure HW SPI = SPI1 Master Mode
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;

    // Used SPI_CS =PC4
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;  // Not used HW NSS
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
    SPI_Init(SPI1, &SPI_InitStructure);
    
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    SPI_Cmd(SPI1, ENABLE);

    RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;    // Enable DMA peripheral
    DMA1_Channel3->CFGR = 0;    // Clear previous configuration

    // Config DMA for SPI TX in Circular Mode
    DMA1_Channel3->CFGR = DMA_DIR_PeripheralDST          // Bit 4     - Read from memory
                          | DMA_Mode_Circular            // Bit 5     - Circulation mode
                          | DMA_PeripheralInc_Disable    // Bit 6     - Peripheral address no change
                          | DMA_MemoryInc_Enable         // Bit 7     - Increase memory address
                          | DMA_PeripheralDataSize_Byte  // Bit 8-9   - 8-bit data
                          | DMA_MemoryDataSize_Byte      // Bit 10-11 - 8-bit data
                          | DMA_Priority_VeryHigh        // Bit 12-13 - Very high priority
                          | DMA_M2M_Disable;             // Bit 14    - Disable memory to memory mode
    DMA1_Channel3->PADDR = (uint32_t)&SPI1->DATAR;  // Set Peripheral address
}

// Send Data Through SPI via DMA
// buffer Memory address, size Memory size, repeat Repeat times
static void SPI_send_DMA(const uint8_t* buffer, uint16_t size, uint16_t repeat)
{
    // Set memory address and data count
    DMA1_Channel3->MADDR = (uint32_t)buffer; // Set memory address
    DMA1_Channel3->CNTR  = size;              // Set number of data items

    // Circulate the buffer
    for (uint16_t i = 0; i < repeat; i++)
    {
        DMA1_Channel3->CFGR |= DMA_CFGR1_EN;  // Ensure DMA is enabled

        // Clear flag before starting a new transfer
        DMA1->INTFCR |= DMA1_FLAG_TC3; // Clear transfer complete flag
        while (!(DMA1->INTFR & DMA1_FLAG_TC3)); // Wait until the transfer is complete
    }

    // Disable the DMA channel after transfer
    DMA1_Channel3->CFGR &= ~DMA_CFGR1_EN; // Turn off channel
}

//-------------------------------------------------------------
// brief Send Data Directly Through SPI
// param data 8-bit data
//-------------------------------------------------------------
static void SPI_send8(uint8_t data)
{
	while((SPI1->STATR & SPI_STATR_TXE) != SPI_STATR_TXE){};
    SPI1->DATAR = data; // Send byte

    // Waiting for transmission complete
    //while (!(SPI1->STATR & SPI_STATR_TXE));
    while((SPI1->STATR & SPI_STATR_BSY) == SPI_STATR_BSY){};
 }

void spi_send16(uint16_t data)
{
	while((SPI1->STATR & SPI_STATR_TXE) != SPI_STATR_TXE){};
	SPI1->DATAR = data;
	while((SPI1->STATR & SPI_STATR_BSY) == SPI_STATR_BSY){};
}

uint8_t spi_recv8(uint8_t dummy)
{
	SPI1->DATAR = dummy;
	while((SPI1->STATR & SPI_STATR_RXNE) != SPI_STATR_RXNE){};
	
	return (uint8_t)SPI1->DATAR;
}

void spi_send_dma16(uint16_t *data, uint16_t size)
{
	SPI_DATA_16B(); //Change data length to 16bit

	DMA1_Channel3->CFGR &= ~DMA_CFGR3_EN;   //First disable DMA
	DMA1_Channel3->MADDR = (uint32_t)data;  //Buffer address
	DMA1_Channel3->CNTR = (uint16_t)size;   //Number of data transfer
	DMA1_Channel3->CFGR |= DMA_CFGR3_EN;	//Enable DMA Channel
}   // End of spi_send from spi.c

//-------------------------------------------------------------
// brief Send 8-Bit Command
// param cmd 8-bit command
//-------------------------------------------------------------
static void write_command_8(uint8_t cmd)
{
	SPI_DATA_8B();
    GPIO_ResetBits(GPIOC, GPIO_Pin_3);    // DC = low

	GPIO_ResetBits(GPIOC, SPI_CS);	// ILI9341_CS_ON();
    SPI_send8(cmd);
    //GPIO_SetBits(GPIOC, SPI_CS);	// ILI9341_CS_OFF();
}

/// \brief Send 8-Bit Data
/// \param cmd 8-bit data
static void write_data_8(uint8_t data)
{
	SPI_DATA_8B();
    GPIO_SetBits(GPIOC, GPIO_Pin_3);    // DC = high

	GPIO_ResetBits(GPIOC, SPI_CS);	// ILI9341_CS_ON();
    SPI_send8(data);
    //GPIO_SetBits(GPIOC, SPI_CS);	// ILI9341_CS_OFF();
}

/// \brief Send 16-Bit Data
/// \param cmd 16-bit data
static void write_data_16(uint16_t data)
{
	SPI_DATA_16B(); // <--- Bug ???
    GPIO_SetBits(GPIOC, SPI_DC);    // DC = high

	GPIO_ResetBits(GPIOC, SPI_CS);	// ILI9341_CS_ON();
    //SPI_send8(data >> 8);
    //SPI_send8(data);
	spi_send16(data);
    //GPIO_SetBits(GPIOC, SPI_CS);	// ILI9341_CS_OFF();
 }

void write_dma_data16(uint16_t *data, uint16_t size)
{
	GPIO_SetBits(GPIOC, SPI_DC);	// ILI9341_DC_DATA();

	GPIO_ResetBits(GPIOC, SPI_CS);	// ILI9341_CS_ON();
	spi_send_dma16(data, size); //Send data

	while((DMA1->INTFR & DMA_TCIF3) != DMA_TCIF3){};	//Wait end of transfer
	DMA1->INTFCR |= DMA_CGIF3;	//Clear DMA global flag  
}

// ST7735 Gamma Adjustments (pos. polarity), 16 args.
const uint8_t gamma_p[] = {0x09, 0x16, 0x09, 0x20, 0x21, 0x1B, 0x13, 0x19,
                    0x17, 0x15, 0x1E, 0x2B, 0x04, 0x05, 0x02, 0x0E};

// ST7735  Gamma Adjustments (neg. polarity), 16 args.
const uint8_t gamma_n[] = {0x0B, 0x14, 0x08, 0x1E, 0x22, 0x1D, 0x18, 0x1E,
                    0x1B, 0x1A, 0x24, 0x2B, 0x06, 0x06, 0x02, 0x0F};

/// \details Initialization sequence from Arduino_GFX
void tft_init(void)
{
    SPI_init();
    GPIO_ResetBits(GPIOC, GPIO_Pin_4);   // CS = low

    // Out of sleep mode, no args, w/delay
    write_command_8(ILI9341_SLPOUT);
    Delay_Ms(ILI9341_SLPOUT_DELAY);

    // Power control VRH[5:0]
    write_command_8(ILI9341_PWCTR1);
    write_data_8(0x23);             

    // Power control SAP[2:0];BT[3:0]
    write_command_8(ILI9341_PWCTR2);
    write_data_8(0x10);           

    // VCM control 1
    write_command_8(ILI9341_VMCTR1);
    write_data_8(0x3e);
    write_data_8(0x28);       

    // VCM control 2
    write_command_8(ILI9341_VMCTR2);
    write_data_8(0x86);             

    // 0x36 =Memory Data Access Control for Set rotation (ILI9341)
    write_command_8(ILI9341_MADCTL); 
    write_data_8(ILI9341_MADCTL_MX | ILI9341_MADCTL_MY |ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR ); // 0 - Horizontal

    // Set rotate for ST7735 =0 ~ 3
    //write_data_8(ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR); // 0 - Horizontal
    //write_data_8(ILI9341_MADCTL_BGR); // 1 - Vertical
    //write_data_8(ILI9341_MADCTL_MX | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR); // 2 - Horizontal
    //write_data_8(ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR); // 3 - Vertical   

    // Set Interface Pixel Format = 16 bit/pixel
    write_command_8(ILI9341_COLMOD);
    write_data_8(ILI9341_COLMOD_16_BPP);

    // Vertical Scrolling Start Address
    write_command_8(0x37);
    write_data_8(0x00);             

    //COLMOD: Pixel Format Set
    write_command_8(ILI9341_COLMOD);
    write_data_8(0x55);

    // Frame Rate Control 1
    write_command_8(ILI9341_FRMCTR1);
    write_data_8(0x00);
    write_data_8(0x18);
    
    // Display Function Control
    write_command_8(ILI9341_DFUNCTR);
    write_data_8(0x08);
    write_data_8(0x82);
    write_data_8(0x27);

    // ILI9341_GAMMASET
    write_command_8(ILI9341_GAMSET);
    write_data_8(0x01);

    write_command_8(ILI9341_GMCTRP1); 
    GPIO_SetBits(GPIOC, GPIO_Pin_3);    // DC = high =data
    SPI_send_DMA(gamma_p, 16, 1);   // DMA transfer only data
    Delay_Ms(10);

    write_command_8(ILI9341_GMCTRN1);
    GPIO_SetBits(GPIOC, GPIO_Pin_3);    // DC = high =data
    SPI_send_DMA(gamma_n, 16, 1);   // DMA transfer only data 
    Delay_Ms(10);

	write_command_8(ILI9341_FRMCTR1);   //Frame rate
	write_data_8(0xA0);    //60Hz

    // Color invert
    //write_command_8(ILI9341_INVON);
    write_command_8(ILI9341_INVOFF);

    // Normal display on, no args, w/delay
    write_command_8(ILI9341_NORON);
    Delay_Ms(10);

    // Main screen turn on, no args, w/delay
    write_command_8(ILI9341_DISPON);
    Delay_Ms(10);

	write_command_8(ILI9341_RAMWR); // 0x2C =Memory Write
    GPIO_SetBits(GPIOC, GPIO_Pin_4);    // CS = high
}

void tft_cursor_position(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) 
{
	write_command_8(0x2A);  // ILI9341_COLUMN_ADDR
	write_data_16(x1);
	write_data_16(x2);

	write_command_8(0x2B);  // ILI9341_PAGE_ADDR
	write_data_16(y1);
	write_data_16(y2);
}

/// \brief Set Cursor Position for Print Functions
/// \param x X coordinate, from left to right.
/// \param y Y coordinate, from top to bottom.
/// \details Calculate offset and set to `_cursor_x` and `_cursor_y` variables
void tft_set_cursor(uint16_t x, uint16_t y)
{
    _cursor_x = x + ILI9341_X_OFFSET;
    _cursor_y = y + ILI9341_Y_OFFSET;
}

/// \brief Set Text Color
/// \param color Text color
/// \details Set to `_color` variable
void tft_set_color(uint16_t color)
{
    _color = color;
}

/// \brief Set Text Background Color
/// \param color Text background color
/// \details Set to `_bg_color` variable
void tft_set_background_color(uint16_t color)
{
    _bg_color = color;
}

/// \brief Set Memory Write Window
/// \param x0 Start column
/// \param y0 Start row
/// \param x1 End column
/// \param y1 End row
static void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    write_command_8(ILI9341_CASET);
    write_data_16(x0);
    write_data_16(x1);

    write_command_8(ILI9341_RASET);
    write_data_16(y0);
    write_data_16(y1);

    write_command_8(ILI9341_RAMWR);
}

/// \brief Print a Character
/// \param c Character to print
/// \details DMA accelerated.
void tft_print_char(char c)
{
    const unsigned char* start = &font[c +(c << 2)];

    uint16_t sz = 0;
    for (uint16_t i =0; i < FONT_HEIGHT; i++)
    {
        for (uint16_t j =0; j < FONT_WIDTH; j++)
        {
            if ((*(start +j)) & (0x01 << i))
            {
                _buffer[sz++] = _color >> 8;
                _buffer[sz++] = _color;
            }
            else
            {
                _buffer[sz++] = _bg_color >> 8;
                _buffer[sz++] = _bg_color;
            }
        }
    }

    GPIO_ResetBits(GPIOC, GPIO_Pin_4);   //START_WRITE();
    tft_set_window(_cursor_x, _cursor_y, _cursor_x +FONT_WIDTH -1, _cursor_y +FONT_HEIGHT -1);

    GPIO_SetBits(GPIOC, GPIO_Pin_3);    //DATA_MODE();
    SPI_send_DMA(_buffer, sz, 1);
    GPIO_SetBits(GPIOC, GPIO_Pin_4);    //END_WRITE();
}

//#include "fonts.h"
//TM_FontDef_t TM_Font_7x10;

//DMA buffer for putc function
uint16_t buffer[512];

void tft_get_font_size(char *str, TM_FontDef_t *font, uint16_t *width, uint16_t *height) 
{
	uint16_t w = 0;
	*height = font->FontHeight;
	while (*str++) 
    {
		w += font->FontWidth;
	}
	*width = w;
}

void tft_putc(uint16_t x, uint16_t y, char c, TM_FontDef_t *font, uint32_t foreground, uint32_t background) 
{
	uint32_t i, b, j;

    /* Set coordinates */
	ili9341_x =x;
	ili9341_y =y;

	if ((ili9341_x + font->FontWidth) > ILI9341.width) 
    {
		//If at the end of a line of display, go to new line and set x to 0 position
		ili9341_y += font->FontHeight;
		ili9341_x =0;
	}
	for (i = 0; i < font->FontHeight; i++) 
    {
		b = font->data[(c -32) *font->FontHeight + i];

		for (j =0; j < font->FontWidth; j++) 
        {
			if ((b << j) & 0x8000) 
            {
				buffer[j +(i *font->FontWidth)] =foreground;
			} 
            else if ((background & ILI9341_TRANSPARENT) ==0) 
            {
				buffer[j +(i *font->FontWidth)] =background;
			}
		}
	}

	tft_cursor_position(ili9341_x,ili9341_y, (ili9341_x +font->FontWidth -1), (ili9341_y+font->FontHeight - 1));
	write_command_8(ILI9341_RAMWR);  // RAM WRITE =0x2CC
	write_command_8(ILI9341_WRTCONT);  // WRITE_CONTINUE =0x3C
    
	SPI_DMA_MEM_INC_ON();
	write_dma_data16(buffer, (font->FontWidth *font->FontHeight));
	SPI_DMA_MEM_INC_OFF();
	ili9341_x += font->FontWidth;
}

void tft_puts(uint16_t x, uint16_t y, char *str, TM_FontDef_t *font, uint32_t foreground, uint32_t background) 
{
	uint16_t startX = x;

	/* Set X and Y coordinates */
	ili9341_x = x;
	ili9341_y = y;

	while (*str) 
    {
		//New line
		if (*str == '\n') 
        {
			ili9341_y += font->FontHeight + 1;
			//if after \n is also \r, than go to the left of the screen
			if (*(str + 1) == '\r') 
            {
				ili9341_x = 0;
				str++;
			} 
            else 
            {
				ili9341_x = startX;
			}
			str++;
			continue;
		} 
        else if (*str == '\r') 
        {
			str++;
			continue;
		}
		tft_putc(ili9341_x, ili9341_y, *str++, font, foreground, background);
	}
}

/// \brief Print a String
/// \param str String to print
void tft_print(const char* str)
{
    while (*str)
    {
        tft_print_char(*str++);
        _cursor_x += FONT_WIDTH +1;
    }
}

/// \brief Print an Integer
/// \param num Number to print
/// \param width Expected width of the number.
/// Align left if it is less than the width of the number.
/// Align right if it is greater than the width of the number.
void tft_print_number(int32_t num, uint16_t width)
{
    static char str[12];
    uint8_t     position  = 11;
    uint8_t     negative  = 0;
    uint16_t    num_width = 0;

    // Handle negative number
    if (num < 0)
    {
        negative = 1;
        num      = -num;
    }

    str[position] = '\0';  // End of the string.
    while (num)
    {
        str[--position] = num % 10 + '0';
        num /= 10;
    }

    if (position == 11)
    {
        str[--position] = '0';
    }

    if (negative)
    {
        str[--position] = '-';
    }

    // Calculate alignment
    num_width = (11 - position) *(FONT_WIDTH +1) -1;
    if (width > num_width)
    {
        _cursor_x += width -num_width;
    }
    tft_print(&str[position]);
}

/// \brief Draw a Pixel
/// \param x X
/// \param y Y
/// \param color Pixel color
/// \details SPI direct write
void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    x += ILI9341_X_OFFSET;
    y += ILI9341_Y_OFFSET;

    GPIO_ResetBits(GPIOC, GPIO_Pin_4);   //START_WRITE();
    tft_set_window(x, y, x, y);
    write_data_16(color);
    GPIO_SetBits(GPIOC, GPIO_Pin_4);    //END_WRITE();
}

/// \brief Fill a Rectangle Area
/// \param x Start X coordinate
/// \param y Start Y coordinate
/// \param width Width
/// \param height Height
/// \param color Fill Color
/// \details DMA accelerated.
void tft_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    x += ILI9341_X_OFFSET;
    y += ILI9341_Y_OFFSET;

    uint16_t sz = 0;
    for (uint16_t x = 0; x < width; x++)
    {
        _buffer[sz++] = color >> 8;
        _buffer[sz++] = color;
    }

    GPIO_ResetBits(GPIOC, GPIO_Pin_4);   //START_WRITE();
    tft_set_window(x, y, x + width - 1, y + height - 1);
    GPIO_SetBits(GPIOC, GPIO_Pin_3);    //DATA_MODE();
    SPI_send_DMA(_buffer, sz, height);
    GPIO_SetBits(GPIOC, GPIO_Pin_4);    //END_WRITE();
}

/// \brief Draw a Bitmap
/// \param x Start X coordinate
/// \param y Start Y coordinate
/// \param width Width
/// \param height Height
/// \param bitmap Bitmap
void tft_draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* bitmap)
{
    x += ILI9341_X_OFFSET;
    y += ILI9341_Y_OFFSET;

    GPIO_SetBits(GPIOC, GPIO_Pin_3);    //DATA_MODE();
    GPIO_ResetBits(GPIOC, GPIO_Pin_4);  //START_WRITE();
    tft_set_window(x, y, x + width -1, y + height -1);
    SPI_send_DMA(bitmap, width * height << 1, 1);
    GPIO_SetBits(GPIOC, GPIO_Pin_4);    //END_WRITE();
}

/// \brief Draw a Vertical Line Fast
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
/// \details DMA accelerated
static void _tft_draw_fast_v_line(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    x += ILI9341_X_OFFSET;
    y += ILI9341_Y_OFFSET;

    uint16_t sz = 0;
    for (int16_t j = 0; j < h; j++)
    {
        _buffer[sz++] = color >> 8;
        _buffer[sz++] = color;
    }

    GPIO_ResetBits(GPIOC, GPIO_Pin_4);   //START_WRITE();
    tft_set_window(x, y, x, y + h - 1);
    GPIO_SetBits(GPIOC, GPIO_Pin_3);     // DATA_MODE();   
    SPI_send_DMA(_buffer, sz, 1);
    GPIO_SetBits(GPIOC, GPIO_Pin_4);    //END_WRITE();
}

/// \brief Draw a Horizontal Line Fast
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
/// \details DMA accelerated
static void _tft_draw_fast_h_line(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    x += ILI9341_X_OFFSET;
    y += ILI9341_Y_OFFSET;

    uint16_t sz = 0;
    for (int16_t j = 0; j < w; j++)
    {
        _buffer[sz++] = color >> 8;
        _buffer[sz++] = color;
    }
    
    GPIO_ResetBits(GPIOC, GPIO_Pin_4);   //START_WRITE();
    tft_set_window(x, y, x +w -1, y);
    GPIO_SetBits(GPIOC, GPIO_Pin_3);    //DATA_MODE();
    SPI_send_DMA(_buffer, sz, 1);
    GPIO_SetBits(GPIOC, GPIO_Pin_4);    //END_WRITE();
}

// Draw line helpers
#define _diff(a, b) ((a > b) ? (a - b) : (b - a))
#define _swap_int16_t(a, b) \
    {                       \
        int16_t t = a;      \
        a         = b;      \
        b         = t;      \
    }

/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
/// \details SPI direct write
static void _tft_draw_line_bresenham(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    uint8_t steep = _diff(y1, y0) > _diff(x1, x0);
    if (steep)
    {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1)
    {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx   = x1 - x0;
    int16_t dy   = _diff(y1, y0);
    int16_t err  = dx >> 1;
    int16_t step = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++)
    {
        if (steep)
        {
            tft_draw_pixel(y0, x0, color);
        }
        else
        {
            tft_draw_pixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0)
        {
            err += dx;
            y0 += step;
        }
    }
}

/// \brief Draw a Rectangle
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
void tft_draw_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    _tft_draw_fast_h_line(x, y, width, color);
    _tft_draw_fast_h_line(x, y + height - 1, width, color);
    _tft_draw_fast_v_line(x, y, height, color);
    _tft_draw_fast_v_line(x + width - 1, y, height, color);
}

/// \brief Draw Line Function from Arduino GFX
/// https://github.com/moononournation/Arduino_GFX/blob/master/src/Arduino_GFX.cpp
/// \param x0 Start X coordinate
/// \param y0 Start Y coordinate
/// \param x1 End X coordinate
/// \param y1 End Y coordinate
/// \param color Line color
void tft_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    if (x0 == x1)
    {
        if (y0 > y1)
        {
            _swap_int16_t(y0, y1);
        }
        _tft_draw_fast_v_line(x0, y0, y1 - y0 + 1, color);
    }
    else if (y0 == y1)
    {
        if (x0 > x1)
        {
            _swap_int16_t(x0, x1);
        }
        _tft_draw_fast_h_line(x0, y0, x1 - x0 + 1, color);
    }
    else
    {
        _tft_draw_line_bresenham(x0, y0, x1, y1, color);
    }
}

void tft_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) 
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    tft_draw_pixel(x0, y0 + r, color);  // 180 deg point
    tft_draw_pixel(x0, y0 - r, color);  // 360 deg point
    tft_draw_pixel(x0 + r, y0, color);  // 90 deg point
    tft_draw_pixel(x0 - r, y0, color);  // 270 deg point

    // for (x =0; x < r; (x++; f +=2))
    while (x < y) 
	{
        // for (y =r; y+2 >0; (y--; f +=2)) 
        if (f >= 0) 
		{
            y--;    // y = y -1
            ddF_y += 2;
            f += ddF_y; // f = f +2 
        }
        x++;    // x = x +1
        ddF_x += 2;
        f += ddF_x; // f = f +2
        
        // x =(0~r), y =(r ~0)
        tft_draw_pixel(x0 +x, y0 +y, color);  // 135~180
        tft_draw_pixel(x0 -x, y0 +y, color);  // 225~180
        tft_draw_pixel(x0 +x, y0 -y, color);  // 360~45
        tft_draw_pixel(x0 -x, y0 -y, color);  // 315~360 

        tft_draw_pixel(x0 +y, y0 +x, color);
        tft_draw_pixel(x0 -y, y0 +x, color);
        tft_draw_pixel(x0 +y, y0 -x, color);
        tft_draw_pixel(x0 -y, y0 -x, color);
    }
}

void tft_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) 
{
    int x = r;
    int y = 0;
    int xChange = 1 - (r << 1);
    int yChange = 0;
    int radiusError = 0;

    while (x >= y)
    {
        for (int i = x0 - x; i <= x0 + x; i++)
        {
            tft_draw_pixel(i, y0 +y, color);
            tft_draw_pixel(i, y0 -y, color);
        }
        for (int i = x0 -y; i <= x0 +y; i++)
        {
            tft_draw_pixel(i, y0 +x, color);
            tft_draw_pixel(i, y0 -x, color);
        }

        y++;
        radiusError += yChange;
        yChange += 2;
        if (((radiusError << 1) + xChange) > 0)
        {
            x--;
            radiusError += xChange;
            xChange += 2;
        }
    }
}
