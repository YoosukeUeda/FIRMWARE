/* Created by Abhay Dutta, 3rd June, 2024 */
#include "pinout.h"
#include "font.h"

uint8_t disp1color_buff[1024];


/* setup below is as follows
 * A5 ---------> SCLK (EN)
 * A6 ---------> CS (RS)
 * A7 ---------> SID (RW)
 * B0 ---------> RST (RST)
 *
 */


uint8_t startRow, startCol, endRow, endCol; // coordinates of the dirty rectangle
uint8_t numRows = 64;
uint8_t numCols = 128;
uint8_t Graphic_Check = 0;


// A replacement for SPI_TRANSMIT
void SendByteSPI(uint8_t byte)
{
	for(int i=0;i<8;i++)
	{
		if((byte<<i)&0x80)
		{
			gpio_set_level(SID_PIN, GPIO_PIN_SET);  // SID=1  OR MOSI
		}
		else 
		{
			gpio_set_level(SID_PIN, GPIO_PIN_RESET);  // SID=0
		}
		gpio_set_level(SCLK_PIN, GPIO_PIN_RESET);  // SCLK =0  OR SCK
		gpio_set_level(SCLK_PIN, GPIO_PIN_SET);  // SCLK=1
	}
}

void ST7920_SendCmd (uint8_t cmd)
{
	gpio_set_level(CS_PIN, GPIO_PIN_SET);  // PUll the CS high
	SendByteSPI(0xf8+(0<<1));  // send the SYNC + RS(0)
	SendByteSPI(cmd&0xf0);  // send the higher nibble first
	SendByteSPI((cmd<<4)&0xf0);  // send the lower nibble
	esp_rom_delay_us(10);
	gpio_set_level(CS_PIN, GPIO_PIN_RESET);  // PUll the CS LOW
}

void ST7920_SendData (uint8_t data)
{
	gpio_set_level(CS_PIN, GPIO_PIN_SET);  // PUll the CS high
	SendByteSPI(0xf8+(1<<1));  // send the SYNC + RS(1)
	SendByteSPI(data&0xf0);  // send the higher nibble first
	SendByteSPI((data<<4)&0xf0);  // send the lower nibble
	esp_rom_delay_us(10);
	gpio_set_level(CS_PIN, GPIO_PIN_RESET);  // PUll the CS LOW
}


// switch to graphic mode or normal mode::: enable = 1 -> graphic mode enable = 0 -> normal mode

void ST7920_GraphicMode (int enable)   // 1-enable, 0-disable
{
	if (enable == 1)
	{
		ST7920_SendCmd(0x30);  // 8 bit mode
		vTaskDelay(10 / portTICK_PERIOD_MS);
		ST7920_SendCmd(0x34);  // switch to Extended instructions
		vTaskDelay(10 / portTICK_PERIOD_MS);
		ST7920_SendCmd(0x36);  // enable graphics
		vTaskDelay(10 / portTICK_PERIOD_MS);
		Graphic_Check = 1;  // update the variable
	}

	else if (enable == 0)
	{
		ST7920_SendCmd(0x30);  // 8 bit mode
		vTaskDelay(10 / portTICK_PERIOD_MS);
		Graphic_Check = 0;  // update the variable
	}
}

void ST7920_DrawBitmap(const unsigned char* graphic)
{
	uint8_t x, y;
	for(y = 0; y < 64; y++)
	{
		if(y < 32)
		{
			for(x = 0; x < 8; x++)							// Draws top half of the screen.
			{												// In extended instruction mode, vertical and horizontal coordinates must be specified before sending data in.
				ST7920_SendCmd(0x80 | y);				// Vertical coordinate of the screen is specified first. (0-31)
				ST7920_SendCmd(0x80 | x);				// Then horizontal coordinate of the screen is specified. (0-8)
				ST7920_SendData(graphic[2 * x + 16 * y]);		// Data to the upper byte is sent to the coordinate.
				ST7920_SendData(graphic[2 * x + 1 + 16 * y]);	// Data to the lower byte is sent to the coordinate.
			}

		}
		else
		{
			for(x = 0; x < 8; x++)							// Draws bottom half of the screen.
			{												// Actions performed as same as the upper half screen.
				ST7920_SendCmd(0x80 | (y-32));			// Vertical coordinate must be scaled back to 0-31 as it is dealing with another half of the screen.
				ST7920_SendCmd(0x88 | x);
				ST7920_SendData(graphic[2 * x + 16 * y]);
				ST7920_SendData(graphic[2 * x + 1 + 16 * y]);
			}
		}

	}
}


// Update the display with the selected graphics
void ST7920_Update(void)
{
	ST7920_DrawBitmap(disp1color_buff);
}



void ST7920_Clear()
{
	if (Graphic_Check == 1)  // if the graphic mode is set
	{
		uint8_t x, y;
		for(y = 0; y < 64; y++)
		{
			if(y < 32)
			{
				ST7920_SendCmd(0x80 | y);
				ST7920_SendCmd(0x80);
			}
			else
			{
				ST7920_SendCmd(0x80 | (y-32));
				ST7920_SendCmd(0x88);
			}
			for(x = 0; x < 8; x++)
			{
				ST7920_SendData(0);
				ST7920_SendData(0);
			}
		}
	}

	else
	{
		ST7920_SendCmd(0x01);   // clear the display using command
		// delay >1.6 ms
		vTaskDelay(2 / portTICK_PERIOD_MS);
	}
}

void ST7920_Init (void)
{
	gpio_set_level(RST_PIN, GPIO_PIN_RESET);  // RESET=0
	// wait for 10ms
	vTaskDelay(10 / portTICK_PERIOD_MS);
	gpio_set_level(RST_PIN, GPIO_PIN_SET);  // RESET=1

	//wait for >40 ms
	vTaskDelay(50 / portTICK_PERIOD_MS);

	ST7920_SendCmd(0x30);  // 8bit mode
	//  >100us delay
	// vTaskDelay(110 / portTICK_PERIOD_MS);
	esp_rom_delay_us(110);

	ST7920_SendCmd(0x30);  // 8bit mode
	// >37us delay
	// vTaskDelay(40 / portTICK_PERIOD_MS);
	esp_rom_delay_us(40);

	ST7920_SendCmd(0x08);  // D=0, C=0, B=0
	// >100us delay
	// vTaskDelay(110 / portTICK_PERIOD_MS);
	esp_rom_delay_us(110);

	ST7920_SendCmd(0x01);  // clear screen
	// >10 ms delay
	vTaskDelay(12 / portTICK_PERIOD_MS);


	ST7920_SendCmd(0x06);  // cursor increment right no shift
	// 1ms delay
	vTaskDelay(1 / portTICK_PERIOD_MS);

	ST7920_SendCmd(0x0C);  // D=1, C=0, B=0
	// 1ms delay
    vTaskDelay(1 / portTICK_PERIOD_MS);

	ST7920_SendCmd(0x02);  // return to home
	// 1ms delay
	vTaskDelay(1 / portTICK_PERIOD_MS);

}



// set Pixel

void SetPixel(uint8_t x, uint8_t y)
{
  if (y < numRows && x < numCols)
  {
    uint8_t *p = disp1color_buff + ((y * (numCols/8)) + (x/8));
    *p |= 0x80u >> (x%8);

    *disp1color_buff = *p;

    // Change the dirty rectangle to account for a pixel being dirty (we assume it was changed)
    if (startRow > y) { startRow = y; }
    if (endRow <= y)  { endRow = y + 1; }
    if (startCol > x) { startCol = x; }
    if (endCol <= x)  { endCol = x + 1; }
  }
}

/* draw a line
 * start point (X0, Y0)
 * end point (X1, Y1)
 */
void DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  int dx = (x1 >= x0) ? x1 - x0 : x0 - x1;
  int dy = (y1 >= y0) ? y1 - y0 : y0 - y1;
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  for (;;)
  {
    disp1color_DrawPixel(x0, y0,1);
    if (x0 == x1 && y0 == y1) break;
    int e2 = err + err;
    if (e2 > -dy)
    {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}




/* Draw rectangle
 * start point (x,y)
 * w -> width
 * h -> height
 */
void DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	/* Check input parameters */
	if (
		x >= numCols ||
		y >= numRows
	) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= numCols) {
		w = numCols - x;
	}
	if ((y + h) >= numRows) {
		h = numRows - y;
	}

	/* Draw 4 lines */
	DrawLine(x, y, x + w, y);         /* Top line */
	DrawLine(x, y + h, x + w, y + h); /* Bottom line */
	DrawLine(x, y, x, y + h);         /* Left line */
	DrawLine(x + w, y, x + w, y + h); /* Right line */
}




/* Draw filled rectangle
 * Start point (x,y)
 * w -> width
 * h -> height
 */
void DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	uint8_t i;

	/* Check input parameters */
	if (
		x >= numCols ||
		y >= numRows
	) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= numCols) {
		w = numCols - x;
	}
	if ((y + h) >= numRows) {
		h = numRows - y;
	}

	/* Draw lines */
	for (i = 0; i <= h; i++) {
		/* Draw lines */
		DrawLine(x, y + i, x + w, y + i);
	}
}




/* draw circle
 * centre (x0,y0)
 * radius = radius
 */
void DrawCircle(uint8_t x0, uint8_t y0, uint8_t radius)
{
  int f = 1 - (int)radius;
  int ddF_x = 1;

  int ddF_y = -2 * (int)radius;
  int x = 0;
  disp1color_DrawPixel(x0, y0 + radius,1);
  disp1color_DrawPixel(x0, y0 - radius,1);
  disp1color_DrawPixel(x0 + radius, y0,1);
  disp1color_DrawPixel(x0 - radius, y0,1);

  int y = radius;
  while(x < y)
  {
    if(f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    disp1color_DrawPixel(x0 + x, y0 + y,1);
    disp1color_DrawPixel(x0 - x, y0 + y,1);
    disp1color_DrawPixel(x0 + x, y0 - y,1);
    disp1color_DrawPixel(x0 - x, y0 - y,1);
    disp1color_DrawPixel(x0 + y, y0 + x,1);
    disp1color_DrawPixel(x0 - y, y0 + x,1);
    disp1color_DrawPixel(x0 + y, y0 - x,1);
    disp1color_DrawPixel(x0 - y, y0 - x,1);
  }
}


// Draw Filled Circle

void DrawFilledCircle(int16_t x0, int16_t y0, int16_t r)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    disp1color_DrawPixel(x0, y0 + r,1);
    disp1color_DrawPixel(x0, y0 - r,1);
    disp1color_DrawPixel(x0 + r, y0,1);
    disp1color_DrawPixel(x0 - r, y0,1);
    DrawLine(x0 - r, y0, x0 + r, y0);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        DrawLine(x0 - x, y0 + y, x0 + x, y0 + y);
        DrawLine(x0 + x, y0 - y, x0 - x, y0 - y);

        DrawLine(x0 + y, y0 + x, x0 - y, y0 + x);
        DrawLine(x0 + y, y0 - x, x0 - y, y0 - x);
    }
}



// Draw Traingle with coordimates (x1, y1), (x2, y2), (x3, y3)
void DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
	/* Draw lines */
	DrawLine(x1, y1, x2, y2);
	DrawLine(x2, y2, x3, y3);
	DrawLine(x3, y3, x1, y1);
}



// Draw Filled Traingle with coordimates (x1, y1), (x2, y2), (x3, y3)
void DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3)
{
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0, yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0, curpixel = 0;

#define ABS(x)   ((x) > 0 ? (x) : -(x))

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	} else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	} else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay){
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++)
	{
		DrawLine(x, y, x3, y3);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}

void pin_init()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,        // Desativa interrupções
        .mode = GPIO_MODE_OUTPUT,              // Configura o pino como saída
        .pin_bit_mask = (1ULL << SCLK_PIN) |
                        (1ULL << CS_PIN) |
                        (1ULL << SID_PIN) |
						(1ULL << RST_PIN), // Configura o pino específico
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Desativa pull-down
        .pull_up_en = GPIO_PULLUP_DISABLE      // Desativa pull-up
    };
    gpio_config(&io_conf);
	esp_rom_gpio_pad_select_gpio(SCLK_PIN);
    esp_rom_gpio_pad_select_gpio(CS_PIN);
    esp_rom_gpio_pad_select_gpio(SID_PIN);
    esp_rom_gpio_pad_select_gpio(RST_PIN);
}

#if 1

void ST7920_Ext_SetGDRAMAddr(uint8_t VertAddr, uint8_t HorizAddr)
{
  uint8_t Data = 0x80;
  Data |= (VertAddr & 0x7F);
  ST7920_SendCmd(Data);
  
  Data = 0x80;
  Data |= (HorizAddr & 0x0F);
  ST7920_SendCmd(Data);

  esp_rom_delay_us(72);
}

uint8_t ST7920_GetHorizontalByte(uint8_t *pBuff, uint8_t Row, uint8_t Col)
{
  uint8_t Byte = 0;
  uint16_t ByteIdx = (Row >> 3) * numCols;       
  ByteIdx += (Col << 3);
  uint8_t BitMask = Row % 8;
  BitMask = (1 << BitMask);
  
  for (uint8_t Bit = 0; Bit < 8; Bit++)
  {
    if (pBuff[ByteIdx + Bit] & BitMask)
      Byte |= (1 << (7 - Bit));
  }
  return Byte;
}

void ST7920_DisplayFullUpdate(uint8_t *pBuff, uint16_t BuffLen)
{
  for (uint8_t Row = 0; Row < 32; Row++)
  {
    ST7920_Ext_SetGDRAMAddr(Row, 0);
    for (uint8_t Col = 0; Col < 16; Col++)
      ST7920_SendData(ST7920_GetHorizontalByte(pBuff, Row, Col));

    for (uint8_t Col = 0; Col < 16; Col++)
      ST7920_SendData(ST7920_GetHorizontalByte(pBuff, Row + 32, Col));
  }
}

void disp1color_UpdateFromBuff(void)
{
  ST7920_DisplayFullUpdate(disp1color_buff, sizeof(disp1color_buff));
}

void disp1color_DrawPixel(int16_t X, int16_t Y, uint8_t State)
{
  if ((X >= numCols) || (Y >= numRows) || (X < 0) || (Y < 0))
    return;
  
  uint16_t ByteIdx = Y >> 3;
  uint8_t BitIdx = Y - (ByteIdx << 3); 
  ByteIdx *= numCols;  	
  ByteIdx += X;
  
  if (State)
    disp1color_buff[ByteIdx] |= (1 << BitIdx);
  else
    disp1color_buff[ByteIdx] &= ~(1 << BitIdx);
}

uint8_t disp1color_DrawChar(int16_t X, int16_t Y, uint8_t FontID, uint8_t Char)
{
  uint8_t *pCharTable = font_GetFontStruct(FontID, Char);
  uint8_t CharWidth = font_GetCharWidth(pCharTable);    
  uint8_t CharHeight = font_GetCharHeight(pCharTable); 
  pCharTable += 2;
  
  if (FontID == FONTID_6X8M)
  {
    for (uint8_t row = 0; row < CharHeight; row++)
    {
      for (uint8_t col = 0; col < CharWidth; col++)
      {
      	disp1color_DrawPixel(X + col, Y + row, pCharTable[row] & (1 << (7 - col)));
		// printf("drawing chars\n");
      }
    }
  }
  else
  {
    for (uint8_t row = 0; row < CharHeight; row++)
    {
      for (uint8_t col = 0; col < CharWidth; col++)
      {
        if (col < 8)
          disp1color_DrawPixel(X + col, Y + row, pCharTable[row * 2] & (1 << (7 - col)));
        else
          disp1color_DrawPixel(X + col, Y + row, pCharTable[(row * 2) + 1] & (1 << (15 - col)));
      }
    }
  }
  return CharWidth;
}

void disp1color_DrawString(int16_t X, int16_t Y, uint8_t FontID, uint8_t *Str)
{
  uint8_t done = 0;             
  int16_t Xstart = X;           
  uint8_t StrHeight = 8;        

  while (!done)
  {
    switch (*Str)
    {
    case '\0':  
      done = 1;
      break;
    case '\n':  
      Y += StrHeight;
      break;
    case '\r':  
      X = Xstart;
      break;
    default:    
      X += disp1color_DrawChar(X, Y, FontID, *Str);
      StrHeight = font_GetCharHeight(font_GetFontStruct(FontID, *Str));
      break;
    }
    Str++;
  }
}



void disp1color_printf(int16_t X, int16_t Y, uint8_t FontID, const char *args, ...)
{
  char StrBuff[100];
  
  va_list ap;
  va_start(ap, args);
  char len = vsnprintf(StrBuff, sizeof(StrBuff), args, ap);
  va_end(ap);
  // printf("before draw\n");
  disp1color_DrawString(X, Y, FontID, (uint8_t *)StrBuff);
}

void clear_buffer(){
  memset(disp1color_buff, 0, sizeof(disp1color_buff));
}

void send_bitmap_to_buffer(const unsigned char *bitmap, 
                           int start_x, int start_y, int width, int height) {
    int x, y;
    int buffer_width = 128; // Assuming the ST7920 standard 128x64 resolution

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            int buffer_index = (start_y + y) * (buffer_width / 8) + (start_x + x) / 8;
            int bit_position = 7 - ((start_x + x) % 8);
            
            if (bitmap[y * ((width + 7) / 8) + x / 8] & (1 << (7 - (x % 8)))) {
                disp1color_buff[buffer_index] |= (1 << bit_position);
            } else {
                disp1color_buff[buffer_index] &= ~(1 << bit_position);
            }
        }
    }
}
// desenha coluna no fim do limite do display
// precisa ter o x0 como a media do rssi
uint16_t last_y = 0;
void graphic(uint32_t rssi){
	uint8_t lenght = 128;
	uint8_t y = 62;
	uint8_t x = 128-6;
	uint16_t graph_height = 0;
	if(rssi <= 23){
		graph_height = rssi;
	} else {
		graph_height = 21;
	}
	if(last_y == 0)
		last_y = graph_height;
	move_graphic();
	disp1color_DrawPixel(x,y - graph_height,1);
	DrawLine(x,y - graph_height,x, y-last_y);
	last_y = graph_height;

}
//manda tudo pra esquerda
void move_graphic(void){
	
	uint16_t value = 1024-(12*128/4);
	//ESP_LOGI(__func__,"teste");
	while (value <= 1024){
		if ( value > 768){
			disp1color_buff[value] = 0b0000000000;
			disp1color_buff[value] = disp1color_buff[value+1];
		}
		else{
			disp1color_buff[value] = disp1color_buff[value+1];
		}
		value += 1;
		clear_last_collum();
	}
}
// limpa a ultima coluna
void clear_last_collum(void) {
	disp1color_buff[1023] = 0b00000000;
	disp1color_buff[895] = 0b00000000;
	disp1color_buff[767] = 0b00000000;
}

#endif
