#ifndef _BK4819_H
#define _BK4819_H
#include "pinout.h"
#define BK4819_DELAY 5

#define CTC_ERR  0 //0=0.1Hz error mode;1=0.1% error mode
#define CTC_IN  10 //~=1   Hz for ctcss in  threshold
#define CTC_OUT 15 //~=1.5 Hz for ctcss out threshold

#define REG_32 0x0244
#define REG_37 0x1D00
#define REG_40 0x3000
#define REG_46 0xA000
#define REG_47 0x6042 //0x6040, 2020.9.18
#define REG_48 0x3000 //0xB000, 2020.9.18
#define REG_4D 0xA000
#define REG_4E 0x6F00
#define REG_50 0x3B18 //0x3B20, 2020.10.15 
#define REG_52 CTC_ERR<<12 | CTC_IN<<6 | CTC_OUT
#define REG_59 0x0028//0x0028
#define REG_7A 0x089A
#define REG_7D 0xE940
#define REG_7E 0x302E

#define DTMF_LEN  15   
unsigned char DTMF_RXDATA[DTMF_LEN];
unsigned char DTMF_SYMBOL[DTMF_LEN];
#define DAC_GAIN  15  //0~15
#define VOL_GAIN  59  //0~63, 0=mute, LSB->0.5dB
#define MIC_GAIN  16 //0~31
#define DTMF_TH   24   //0~63

#define FREQ_697  0x1C1C //697*10.32444; 10.32444 for 13M/26M and 10.48576 for 12.8M/19.2M/25.6M/38.4M
#define FREQ_770  0x1F0E
#define FREQ_852  0x225C
#define FREQ_941  0x25F3
#define FREQ_1209 0x30C2
#define FREQ_1336 0x35E1
#define FREQ_1477 0x3B91
#define FREQ_1633 0x41DC

#define FSKBUAD   0x3065 //1200*10.32444; 10.32444 for 13M/26M and 10.48576 for 12.8M/19.2M/25.6M/38.4M

#define FSK_LEN   5 //0~127, numbers_of_byte=(FSK_LEN*2)
#define FSK2400   0 //0=1200;1=2400
#define MDC_LEN   8 //0~127, numbers_of_byte=(MDC_LEN*2)
#define MDC2400   0 //1=1200/2400; 0=1200/1800

void BK4819_SetAGC(uint8_t Value);
// -----------------------------------------------------------------------------
#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif
static const uint16_t FSK_RogerTable[7] = {0x0102, 0x0304, 0x0506, 0x0708, 0x090A, 0x0B0C, 0x0D0E};
static uint16_t gBK4819_GpioOutState;
bool gRxIdleMode;
// -----------------------------------------------------------------------------
__inline uint32_t scale_freq(const uint32_t freq){
//  return (((uint32_t)freq * 1032444u) + 50000u) / 100000u;   // with rounding
    return (((uint32_t)freq * 1353245u) + (1u << 16)) >> 17;   // with rounding
}
// -----------------------------------------------------------------------------
void BK4819_WriteRegister(BK4819_REGISTER_t Register, uint16_t Data);
void BK4819_WriteRegister(BK4819_REGISTER_t Register, uint16_t Data);
uint16_t BK4819_ReadU16(void);
void BK4819_WriteU8(uint8_t Data);
// -----------------------------------------------------------------------------

void setup_PIN(void){
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,        // Desativa interrupções
        .mode = GPIO_MODE_OUTPUT,              // Configura o pino como saída
        .pin_bit_mask = (1ULL << PIN_BK4819_SCL) |
                        (1ULL << PIN_BK4819_DATA) |
                        (1ULL << PIN_BK4819_SCN), // Configura o pino específico
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Desativa pull-down
        .pull_up_en = GPIO_PULLUP_DISABLE      // Desativa pull-up
    };
    gpio_config(&io_conf);
}

void BK_SetFreq(uint32_t freq)
{
    BK4819_SetFrequency(freq); 
    esp_rom_delay_us(10);
    BK4819_PickRXFilterPathBasedOnFrequency(freq);
    BK4819_RX_TurnOn();// liga rx
    vTaskDelay(10/ portTICK_PERIOD_MS);
    uint32_t test = Read_BK4819_Frequency();
    esp_rom_delay_us(10);
    ESP_LOGI(__func__,"freq = [%ld]",test);
    uint16_t test2 = BK4819_GetRSSI();
    esp_rom_delay_us(10);
    ESP_LOGI(__func__,"rssi = [%d]",test2);
    esp_rom_delay_us(100);
}

void BK4819_Init(void){
    
    esp_rom_gpio_pad_select_gpio(PIN_BK4819_SCN);
    gpio_set_direction(PIN_BK4819_SCN, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(PIN_BK4819_SCL);
    gpio_set_direction(PIN_BK4819_SCL, GPIO_MODE_OUTPUT);
    esp_rom_gpio_pad_select_gpio(PIN_BK4819_DATA);
    gpio_set_direction(PIN_BK4819_DATA, GPIO_MODE_OUTPUT);
    
    gpio_set_level(PIN_BK4819_SCN, 1);
    gpio_set_level(PIN_BK4819_SCL, 1);
    gpio_set_level(PIN_BK4819_DATA, 1);
    esp_rom_gpio_pad_select_gpio(PIN_BK4819_DATA);
    gpio_set_direction(PIN_BK4819_DATA, GPIO_MODE_OUTPUT);
    BK4819_WriteRegister(BK4819_REG_00, 0x8000);
    BK4819_WriteRegister(BK4819_REG_00, 0x0000);
    BK4819_WriteRegister(BK4819_REG_37, 0x1D0F);
    BK4819_WriteRegister(BK4819_REG_36, 0x003F);
//  BK4819_SetAGC(0);
    BK4819_SetAGC(0);
    BK4819_WriteRegister(BK4819_REG_19, 0b0001000001000001);   // <15> MIC AGC  1 = disable  0 = enable
    BK4819_WriteRegister(BK4819_REG_7D, 0xE940);
    // RX AF level
    //
    // REG_48 <15:12> 11  ???  0 to 15
    //
    // REG_48 <11:10> 0 AF Rx Gain-1
    //                0 =   0dB
    //                1 =  -6dB
    //                2 = -12dB
    //                3 = -18dB
    //
    // REG_48 <9:4>   60 AF Rx Gain-2  -26dB ~ 5.5dB   0.5dB/step
    //                63 = max
    //                 0 = mute
    //
    // REG_48 <3:0>   15 AF DAC Gain (after Gain-1 and Gain-2) approx 2dB/step
    //                15 = max
    //                 0 = min
    //
    BK4819_WriteRegister(BK4819_REG_48,	//  0xB3A8);     // 1011 00 111010 1000
    (11u << 12) |     // ??? 0..15
    ( 0u << 10) |     // AF Rx Gain-1
    (58u <<  4) |     // AF Rx Gain-2
    ( 8u <<  0));     // AF DAC Gain (after Gain-1 and Gain-2)
#if 1
    const uint8_t dtmf_coeffs[] = {111, 107, 103, 98, 80, 71, 58, 44, 65, 55, 37, 23, 228, 203, 181, 159};
    for (unsigned int i = 0; i < ARRAY_SIZE(dtmf_coeffs); i++)
            BK4819_WriteRegister(BK4819_REG_09, (i << 12) | dtmf_coeffs[i]);
#else
    // original code
    BK4819_WriteRegister(BK4819_REG_09, 0x006F);  // 6F
    BK4819_WriteRegister(BK4819_REG_09, 0x106B);  // 6B
    BK4819_WriteRegister(BK4819_REG_09, 0x2067);  // 67
    BK4819_WriteRegister(BK4819_REG_09, 0x3062);  // 62
    BK4819_WriteRegister(BK4819_REG_09, 0x4050);  // 50
    BK4819_WriteRegister(BK4819_REG_09, 0x5047);  // 47
    BK4819_WriteRegister(BK4819_REG_09, 0x603A);  // 3A
    BK4819_WriteRegister(BK4819_REG_09, 0x702C);  // 2C
    BK4819_WriteRegister(BK4819_REG_09, 0x8041);  // 41
    BK4819_WriteRegister(BK4819_REG_09, 0x9037);  // 37
    BK4819_WriteRegister(BK4819_REG_09, 0xA025);  // 25
    BK4819_WriteRegister(BK4819_REG_09, 0xB017);  // 17
    BK4819_WriteRegister(BK4819_REG_09, 0xC0E4);  // E4
    BK4819_WriteRegister(BK4819_REG_09, 0xD0CB);  // CB
    BK4819_WriteRegister(BK4819_REG_09, 0xE0B5);  // B5
    BK4819_WriteRegister(BK4819_REG_09, 0xF09F);  // 9F
#endif
    BK4819_WriteRegister(BK4819_REG_1F, 0x5454);
    BK4819_WriteRegister(BK4819_REG_3E, 0xA037);
    gBK4819_GpioOutState = 0x9000;
    BK4819_WriteRegister(BK4819_REG_33, 0x9000);
    BK4819_WriteRegister(BK4819_REG_3F, 0);
}
/*
void BK4819_Init2(void){
    PIN_BK4819_SCN   = 1;
    PIN_BK4819_SCL   = 1;
    PIN_BK4819_SDA   = 1;
    _PIN_DTBK_Pin2Out;

    BK4819_WriteRegister(BK4819_REG_00, 0x8000);
    BK4819_WriteRegister(BK4819_REG_00, 0x8000);
    BK4819_WriteRegister(BK4819_REG_00, 0x0000);
    BK4819_WriteRegister(BK4819_REG_37, 0x9D1F);
    BK4819_WriteRegister(BK4819_REG_10, 0x0318);
    BK4819_WriteRegister(BK4819_REG_11, 0x033A);
    BK4819_WriteRegister(BK4819_REG_12, 0x03DB);
    BK4819_WriteRegister(BK4819_REG_13, 0x03DF);
    BK4819_WriteRegister(BK4819_REG_14, 0x0210);
    BK4819_WriteRegister(BK4819_REG_19, 0x1041);
    BK4819_WriteRegister(BK4819_REG_40, 0x34F0);
    BK4819_WriteRegister(BK4819_REG_46, 0x6050);
    BK4819_WriteRegister(BK4819_REG_49, 0x2AB2);
    BK4819_WriteRegister(BK4819_REG_4A, 0x5430);
    BK4819_WriteRegister(BK4819_REG_7B, 0x73DC);
    BK4819_WriteRegister(BK4819_REG_7D, 0xE920);
    BK4819_WriteRegister(BK4819_REG_7E, 0x303E);
    BK4819_WriteRegister(BK4819_REG_48, 0xB3BF);
    BK4819_WriteRegister(BK4819_REG_4D, 0xA004);
    BK4819_WriteRegister(BK4819_REG_4E, 0x3815);
    BK4819_WriteRegister(BK4819_REG_4F, 0x3F3E);
    BK4819_WriteRegister(BK4819_REG_1C, 0x07C0);
    BK4819_WriteRegister(BK4819_REG_1D, 0xE555);
    BK4819_WriteRegister(BK4819_REG_1E, 0x4C58);
    BK4819_WriteRegister(BK4819_REG_1F, 0x865A);
    BK4819_WriteRegister(BK4819_REG_3A, 0x9A7C);
    BK4819_WriteRegister(BK4819_REG_3E, 0x94C6);
    BK4819_WriteRegister(BK4819_REG_3F, 0x07FE);
    BK4819_WriteRegister(BK4819_REG_73, 0x4681);
    BK4819_WriteRegister(BK4819_REG_77, 0x88EF);
    BK4819_WriteRegister(BK4819_REG_28, 0x0B40);
    BK4819_WriteRegister(BK4819_REG_29, 0xAA00);
    BK4819_WriteRegister(BK4819_REG_2A, 0x6600);
    BK4819_WriteRegister(BK4819_REG_2C, 0x1822);
    BK4819_WriteRegister(BK4819_REG_2F, 0x9890);
    BK4819_WriteRegister(BK4819_REG_53, 0x2028);
    BK4819_WriteRegister(BK4819_REG_25, 0xC1BA);
}

*/

void BK4819_InitAGC(bool amModulation) {
    // REG_10, REG_11, REG_12 REG_13, REG_14
    //
    // Rx AGC Gain Table[]. (Index Max->Min is 3,2,1,0,-1)
    //
    // <15:10> ???
    //
    // <9:8>   LNA Gain Short
    //         3 =   0dB  <<<		1o11				read from spectrum			reference manual
    //         2 = 					-24dB  				-19     					 -11
    //         1 = 					-30dB  				-24     					 -16
    //         0 = 					-33dB  				-28     					 -19
    //
    // <7:5>   LNA Gain
    //         7 =   0dB
    //         6 =  -2dB
    //         5 =  -4dB
    //         4 =  -6dB
    //         3 =  -9dB
    //         2 = -14dB <<<
    //         1 = -19dB
    //         0 = -24dB
    //
    // <4:3>   MIXER Gain
    //         3 =   0dB <<<
    //         2 =  -3dB
    //         1 =  -6dB
    //         0 =  -8dB
    //
    // <2:0>   PGA Gain
    //         7 =   0dB
    //         6 =  -3dB <<<
    //         5 =  -6dB
    //         4 =  -9dB
    //         3 = -15dB
    //         2 = -21dB
    //         1 = -27dB
    //         0 = -33dB
    //

    BK4819_WriteRegister(BK4819_REG_13, 0x03BE);  // 0x03BE / 000000 11 101 11 110 /  -7dB
    BK4819_WriteRegister(BK4819_REG_12, 0x037B);  // 0x037B / 000000 11 011 11 011 / -24dB
    BK4819_WriteRegister(BK4819_REG_11, 0x027B);  // 0x027B / 000000 10 011 11 011 / -43dB
    BK4819_WriteRegister(BK4819_REG_10, 0x007A);  // 0x007A / 000000 00 011 11 010 / -58dB

    if (amModulation) {
        BK4819_WriteRegister(BK4819_REG_14, 0x0000);
        BK4819_WriteRegister(BK4819_REG_49, (0 << 14) | (50 << 7) | (32 << 0));
    } else {
        BK4819_WriteRegister(BK4819_REG_14, 0x0019);  // 0x0019 / 000000 00 000 11 001 / -79dB
        BK4819_WriteRegister(BK4819_REG_49, (0 << 14) | (84 << 7) | (56 << 0)); //0x2A38 / 00 1010100 0111000 / 84, 56
    }
    BK4819_WriteRegister(BK4819_REG_7B, 0x8420);

}
void BK4819_Init3(void) {
    BK4819_WriteRegister(BK4819_REG_00, 0x8000);
    BK4819_WriteRegister(BK4819_REG_00, 0x0000);

    BK4819_WriteRegister(BK4819_REG_37, 0x1D0F);
    BK4819_WriteRegister(BK4819_REG_36, 0x0022);

    BK4819_InitAGC(false);
    BK4819_SetAGC(true);

    BK4819_WriteRegister(BK4819_REG_19, 0b0001000001000001);   // <15> MIC AGC  1 = disable  0 = enable

    BK4819_WriteRegister(BK4819_REG_7D, 0xE940);

    // REG_48 .. RX AF level
    //
    // <15:12> 11  ???  0 to 15
    //
    // <11:10> 0 AF Rx Gain-1
    //         0 =   0dB
    //         1 =  -6dB
    //         2 = -12dB
    //         3 = -18dB
    //
    // <9:4>   60 AF Rx Gain-2  -26dB ~ 5.5dB   0.5dB/step
    //         63 = max
    //          0 = mute
    //
    // <3:0>   15 AF DAC Gain (after Gain-1 and Gain-2) approx 2dB/step
    //         15 = max
    //          0 = min
    //
    BK4819_WriteRegister(BK4819_REG_48,    //  0xB3A8);     // 1011 00 111010 1000
                         (11u << 12) |     // ??? 0..15
                         (0u << 10) |     // AF Rx Gain-1
                         (58u << 4) |     // AF Rx Gain-2
                         (8u << 0));     // AF DAC Gain (after Gain-1 and Gain-2)

#if 1
    const uint8_t dtmf_coeffs[] = {111, 107, 103, 98, 80, 71, 58, 44, 65, 55, 37, 23, 228, 203, 181, 159};
    for (unsigned int i = 0; i < ARRAY_SIZE(dtmf_coeffs); i++)
        BK4819_WriteRegister(BK4819_REG_09, (i << 12) | dtmf_coeffs[i]);
#else
    // original code
    BK4819_WriteRegister(BK4819_REG_09, 0x006F);  // 6F
    BK4819_WriteRegister(BK4819_REG_09, 0x106B);  // 6B
    BK4819_WriteRegister(BK4819_REG_09, 0x2067);  // 67
    BK4819_WriteRegister(BK4819_REG_09, 0x3062);  // 62
    BK4819_WriteRegister(BK4819_REG_09, 0x4050);  // 50
    BK4819_WriteRegister(BK4819_REG_09, 0x5047);  // 47
    BK4819_WriteRegister(BK4819_REG_09, 0x603A);  // 3A
    BK4819_WriteRegister(BK4819_REG_09, 0x702C);  // 2C
    BK4819_WriteRegister(BK4819_REG_09, 0x8041);  // 41
    BK4819_WriteRegister(BK4819_REG_09, 0x9037);  // 37
    BK4819_WriteRegister(BK4819_REG_09, 0xA025);  // 25
    BK4819_WriteRegister(BK4819_REG_09, 0xB017);  // 17
    BK4819_WriteRegister(BK4819_REG_09, 0xC0E4);  // E4
    BK4819_WriteRegister(BK4819_REG_09, 0xD0CB);  // CB
    BK4819_WriteRegister(BK4819_REG_09, 0xE0B5);  // B5
    BK4819_WriteRegister(BK4819_REG_09, 0xF09F);  // 9F
#endif

    BK4819_WriteRegister(BK4819_REG_1F, 0x5454);
    BK4819_WriteRegister(BK4819_REG_3E, 0xA037);

    gBK4819_GpioOutState = 0x9000;

    BK4819_WriteRegister(BK4819_REG_33, 0x9000);
    BK4819_WriteRegister(BK4819_REG_3F, 0);
}
// -----------------------------------------------------------------------------
uint16_t BK4819_ReadU16(void){
    unsigned int i;
    uint16_t     Value;
    esp_rom_gpio_pad_select_gpio(PIN_BK4819_DATA);
    gpio_set_direction(PIN_BK4819_DATA, GPIO_MODE_INPUT);
    Value = 0;
    for (i = 0; i < 16; i++){
        Value <<= 1;
        Value |= gpio_get_level(PIN_BK4819_DATA);
        gpio_set_level(PIN_BK4819_SCL, 1);
        esp_rom_delay_us(BK4819_DELAY);
        gpio_set_level(PIN_BK4819_SCL, 0);
        esp_rom_delay_us(BK4819_DELAY);
    }
    esp_rom_gpio_pad_select_gpio(PIN_BK4819_DATA);
    gpio_set_direction(PIN_BK4819_DATA, GPIO_MODE_OUTPUT);
    return Value;
}
// -----------------------------------------------------------------------------
uint16_t BK4819_ReadRegister(BK4819_REGISTER_t Register){
    uint16_t Value;
    gpio_set_level(PIN_BK4819_SCN, 1);
    gpio_set_level(PIN_BK4819_SCL, 0);
    esp_rom_delay_us(BK4819_DELAY);
    gpio_set_level(PIN_BK4819_SCN, 0);
    BK4819_WriteU8(Register | 0x80);
    Value = BK4819_ReadU16();
    gpio_set_level(PIN_BK4819_SCN, 1);
    esp_rom_delay_us(BK4819_DELAY);
    gpio_set_level(PIN_BK4819_SCL, 1);
    gpio_set_level(PIN_BK4819_DATA, 1);
    return Value;
}
// -----------------------------------------------------------------------------
void BK4819_WriteRegister(BK4819_REGISTER_t Register, uint16_t Data){
    esp_rom_gpio_pad_select_gpio(PIN_BK4819_DATA);
    gpio_set_direction(PIN_BK4819_DATA, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BK4819_SCN, 1);
    gpio_set_level(PIN_BK4819_SCL, 0);
    esp_rom_delay_us(BK4819_DELAY);
    gpio_set_level(PIN_BK4819_SCN, 0);
    BK4819_WriteU8(Register);
    esp_rom_delay_us(BK4819_DELAY);
    BK4819_WriteU16(Data);
    esp_rom_delay_us(BK4819_DELAY);
    gpio_set_level(PIN_BK4819_SCN, 1);
    esp_rom_delay_us(BK4819_DELAY);
    gpio_set_level(PIN_BK4819_SCL, 1);
    gpio_set_level(PIN_BK4819_DATA, 1);
}
// -----------------------------------------------------------------------------
void BK4819_WriteU8(uint8_t Data){
    unsigned int i;
    esp_rom_gpio_pad_select_gpio(PIN_BK4819_DATA);
    gpio_set_direction(PIN_BK4819_DATA, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BK4819_SCL, 0);
    for (i = 0; i < 8; i++){
        if ((Data & 0x80) == 0){
            gpio_set_level(PIN_BK4819_DATA, 0);
        }
        else{
            gpio_set_level(PIN_BK4819_DATA, 1);
        }
        esp_rom_delay_us(BK4819_DELAY);
        gpio_set_level(PIN_BK4819_SCL, 1);
        esp_rom_delay_us(BK4819_DELAY);
        Data <<= 1;
        gpio_set_level(PIN_BK4819_SCL, 0);
        esp_rom_delay_us(BK4819_DELAY);
    }
}
// -----------------------------------------------------------------------------
void BK4819_WriteU16(uint16_t Data){
    unsigned int i;
    esp_rom_gpio_pad_select_gpio(PIN_BK4819_DATA);
    gpio_set_direction(PIN_BK4819_DATA, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BK4819_SCL, 0);
    for (i = 0; i < 16; i++){
        if ((Data & 0x8000) == 0){
            gpio_set_level(PIN_BK4819_DATA, 0);
        }
        else{
            gpio_set_level(PIN_BK4819_DATA, 1);
        }
        esp_rom_delay_us(BK4819_DELAY);
        gpio_set_level(PIN_BK4819_SCL, 1);
        Data <<= 1;
        esp_rom_delay_us(BK4819_DELAY);
        gpio_set_level(PIN_BK4819_SCL, 0);
        esp_rom_delay_us(BK4819_DELAY);
    }
}
// -----------------------------------------------------------------------------
void BK4819_SetAGC(uint8_t Value){
    if (Value == 0){
        // REG_10 <15:0> 0x0038 Rx AGC Gain Table[0]. (Index Max->Min is 3,2,1,0,-1)
        //
        //         <9:8> = LNA Gain Short
        //                 3 =   0dB
        //                 2 = -24dB       // was -11
        //                 1 = -30dB       // was -16
        //                 0 = -33dB       // was -19
        //
        //         <7:5> = LNA Gain
        //                 7 =   0dB
        //                 6 =  -2dB
        //                 5 =  -4dB
        //                 4 =  -6dB
        //                 3 =  -9dB
        //                 2 = -14dB
        //                 1 = -19dB
        //                 0 = -24dB
        //
        //         <4:3> = MIXER Gain
        //                 3 =  0dB
        //                 2 = -3dB
        //                 1 = -6dB
        //                 0 = -8dB
        //
        //         <2:0> = PGA Gain
        //                 7 =   0dB
        //                 6 =  -3dB
        //                 5 =  -6dB
        //                 4 =  -9dB
        //                 3 = -15dB
        //                 2 = -21dB
        //                 1 = -27dB
        //                 0 = -33dB
        //
        // LNA_SHORT ..   0dB
        // LNA ........  14dB
        // MIXER ......   0dB
        // PGA ........  -3dB
        //
        BK4819_WriteRegister(BK4819_REG_13, (3u << 8) | (2u << 5) | (3u << 3) | (6u << 0));  // 000000 11 101 11 110
        BK4819_WriteRegister(BK4819_REG_12, 0x037B);  // 000000 11 011 11 011
        BK4819_WriteRegister(BK4819_REG_11, 0x027B);  // 000000 10 011 11 011
        BK4819_WriteRegister(BK4819_REG_10, 0x007A);  // 000000 00 011 11 010
        BK4819_WriteRegister(BK4819_REG_14, 0x0019);  // 000000 00 000 11 001
        BK4819_WriteRegister(BK4819_REG_49, 0x2A38);
        BK4819_WriteRegister(BK4819_REG_7B, 0x8420);
    }
    else{
        if (Value == 1){
            // what does this do ?????????
            unsigned int i;
            // REG_10
            // <15:0> 0x0038 Rx AGC Gain Table[0]. (Index Max->Min is 3,2,1,0,-1)
            // 
            //  <9:8> = LNA Gain Short
            //          3 =   0dB   << original
            //          2 = -24dB       // was -11
            //          1 = -30dB       // was -16
            //          0 = -33dB       // was -19
            // 
            //  <7:5> = LNA Gain
            //          7 =   0dB
            //          6 =  -2dB
            //          5 =  -4dB
            //          4 =  -6dB
            //          3 =  -9dB
            //          2 = -14dB   << original
            //          1 = -19dB
            //          0 = -24dB
            // 
            //  <4:3> = MIXER Gain
            //          3 =   0dB   << original
            //          2 =  -3dB
            //          1 =  -6dB
            //          0 =  -8dB
            // 
            //  <2:0> = PGA Gain
            //          7 =   0dB
            //          6 =  -3dB   << original
            //          5 =  -6dB
            //          4 =  -9dB
            //          3 = -15dB
            //          2 = -21dB
            //          1 = -27dB
            //          0 = -33dB
            //
            BK4819_WriteRegister(BK4819_REG_13, (3u << 8) | (2u << 5) | (3u << 3) | (6u << 0));
            BK4819_WriteRegister(BK4819_REG_12, 0x037C);  // 000000 11 011 11 100
            BK4819_WriteRegister(BK4819_REG_11, 0x027B);  // 000000 10 011 11 011
            BK4819_WriteRegister(BK4819_REG_10, 0x007A);  // 000000 00 011 11 010
            BK4819_WriteRegister(BK4819_REG_14, 0x0018);  // 000000 00 000 11 000
            BK4819_WriteRegister(BK4819_REG_49, 0x2A38);
            BK4819_WriteRegister(BK4819_REG_7B, 0x318C);
            BK4819_WriteRegister(BK4819_REG_7C, 0x595E);
            BK4819_WriteRegister(BK4819_REG_20, 0x8DEF);
            for (i = 0; i < 8; i++){
                // Bug? The bit 0x2000 below overwrites the (i << 13)
                BK4819_WriteRegister(BK4819_REG_06, ((i << 13) | 0x2500u) + 0x036u);
            }
        }
    }
}
// -----------------------------------------------------------------------------
void BK4819_ToggleGpioOut(BK4819_GPIO_PIN_t Pin, bool bSet){
    if (bSet){
        gBK4819_GpioOutState |=  (0x40u >> Pin);
    }
    else{
        gBK4819_GpioOutState &= ~(0x40u >> Pin);
    }
    BK4819_WriteRegister(BK4819_REG_33, gBK4819_GpioOutState);
}
// -----------------------------------------------------------------------------
void BK4819_SetCDCSSCodeWord(uint32_t CodeWord){
    // REG_51 <15>  0                                 1 = Enable TxCTCSS/CDCSS           0 = Disable
    // REG_51 <14>  0                                 1 = GPIO0Input for CDCSS           0 = Normal Mode.(for BK4819v3)
    // REG_51 <13>  0                                 1 = Transmit negative CDCSS code   0 = Transmit positive CDCSScode
    // REG_51 <12>  0 CTCSS/CDCSS mode selection      1 = CTCSS                          0 = CDCSS
    // REG_51 <11>  0 CDCSS 24/23bit selection        1 = 24bit                          0 = 23bit
    // REG_51 <10>  0 1050HzDetectionMode             1 = 1050/4 Detect Enable, CTC1 should be set to 1050/4 Hz
    // REG_51 <9>   0 Auto CDCSS Bw Mode              1 = Disable                        0 = Enable.
    // REG_51 <8>   0 Auto CTCSS Bw Mode              0 = Enable                         1 = Disable
    // REG_51 <6:0> 0 CTCSS/CDCSS Tx Gain1 Tuning     0 = min                            127 = max
    // Enable CDCSS
    // Transmit positive CDCSS code
    // CDCSS Mode
    // CDCSS 23bit
    // Enable Auto CDCSS Bw Mode
    // Enable Auto CTCSS Bw Mode
    // CTCSS/CDCSS Tx Gain1 Tuning = 51
    //
    BK4819_WriteRegister(BK4819_REG_51,
                      BK4819_REG_51_ENABLE_CxCSS
                    | BK4819_REG_51_GPIO6_PIN2_NORMAL
                    | BK4819_REG_51_TX_CDCSS_POSITIVE
                    | BK4819_REG_51_MODE_CDCSS
                    | BK4819_REG_51_CDCSS_23_BIT
                    | BK4819_REG_51_1050HZ_NO_DETECTION
                    | BK4819_REG_51_AUTO_CDCSS_BW_ENABLE
                    | BK4819_REG_51_AUTO_CTCSS_BW_ENABLE
                    | (51u << BK4819_REG_51_SHIFT_CxCSS_TX_GAIN1));
    // REG_07 <15:0>
    //
    // When <13> = 0 for CTC1
    // <12:0> = CTC1 frequency control word =
    //                          freq(Hz) * 20.64888 for XTAL 13M/26M or
    //                          freq(Hz) * 20.97152 for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    // When <13> = 1 for CTC2 (Tail 55Hz Rx detection)
    // <12:0> = CTC2 (should below 100Hz) frequency control word =
    //                          25391 / freq(Hz) for XTAL 13M/26M or
    //                          25000 / freq(Hz) for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    // When <13> = 2 for CDCSS 134.4Hz
    // <12:0> = CDCSS baud rate frequency (134.4Hz) control word =
    //                          freq(Hz) * 20.64888 for XTAL 13M/26M or
    //                          freq(Hz) * 20.97152 for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    BK4819_WriteRegister(BK4819_REG_07, BK4819_REG_07_MODE_CTC1 | 2775u);
    // REG_08 <15:0> <15> = 1 for CDCSS high 12bit
    //               <15> = 0 for CDCSS low  12bit
    // <11:0> = CDCSShigh/low 12bit code
    //
    BK4819_WriteRegister(BK4819_REG_08, (0u << 15) | ((CodeWord >>  0) & 0x0FFF)); // LS 12-bits
    BK4819_WriteRegister(BK4819_REG_08, (1u << 15) | ((CodeWord >> 12) & 0x0FFF)); // MS 12-bits
}
// -----------------------------------------------------------------------------
void BK4819_SetCTCSSFrequency(uint32_t FreqControlWord){
    // REG_51 <15>  0                                 1 = Enable TxCTCSS/CDCSS           0 = Disable
    // REG_51 <14>  0                                 1 = GPIO0Input for CDCSS           0 = Normal Mode.(for BK4819v3)
    // REG_51 <13>  0                                 1 = Transmit negative CDCSS code   0 = Transmit positive CDCSScode
    // REG_51 <12>  0 CTCSS/CDCSS mode selection      1 = CTCSS                          0 = CDCSS
    // REG_51 <11>  0 CDCSS 24/23bit selection        1 = 24bit                          0 = 23bit
    // REG_51 <10>  0 1050HzDetectionMode             1 = 1050/4 Detect Enable, CTC1 should be set to 1050/4 Hz
    // REG_51 <9>   0 Auto CDCSS Bw Mode              1 = Disable                        0 = Enable.
    // REG_51 <8>   0 Auto CTCSS Bw Mode              0 = Enable                         1 = Disable
    // REG_51 <6:0> 0 CTCSS/CDCSS Tx Gain1 Tuning     0 = min                            127 = max
    uint16_t Config;
    if (FreqControlWord == 2625){
        // Enables 1050Hz detection mode
        // Enable TxCTCSS
        // CTCSS Mode
        // 1050/4 Detect Enable
        // Enable Auto CDCSS Bw Mode
        // Enable Auto CTCSS Bw Mode
        // CTCSS/CDCSS Tx Gain1 Tuning = 74
        //
        Config = 0x944A;   // 1 0 0 1 0 1 0 0 0 1001010
    }
    else{
        // Enable TxCTCSS
        // CTCSS Mode
        // Enable Auto CDCSS Bw Mode
        // Enable Auto CTCSS Bw Mode
        // CTCSS/CDCSS Tx Gain1 Tuning = 74
        //
        Config = 0x904A;   // 1 0 0 1 0 0 0 0 0 1001010
    }
    BK4819_WriteRegister(BK4819_REG_51, Config);
    // REG_07 <15:0>
    //
    // When <13> = 0 for CTC1
    // <12:0> = CTC1 frequency control word =
    //                          freq(Hz) * 20.64888 for XTAL 13M/26M or
    //                          freq(Hz) * 20.97152 for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    // When <13> = 1 for CTC2 (Tail RX detection)
    // <12:0> = CTC2 (should below 100Hz) frequency control word =
    //                          25391 / freq(Hz) for XTAL 13M/26M or
    //                          25000 / freq(Hz) for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    // When <13> = 2 for CDCSS 134.4Hz
    // <12:0> = CDCSS baud rate frequency (134.4Hz) control word =
    //                          freq(Hz) * 20.64888 for XTAL 13M/26M or
    //                          freq(Hz) * 20.97152 for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    BK4819_WriteRegister(BK4819_REG_07, BK4819_REG_07_MODE_CTC1 | (((FreqControlWord * 206488u) + 50000u) / 100000u));   // with rounding
}
// -----------------------------------------------------------------------------
// freq_10Hz is CTCSS Hz * 10
void BK4819_SetTailDetection(const uint32_t freq_10Hz){
    // REG_07 <15:0>
    //
    // When <13> = 0 for CTC1
    // <12:0> = CTC1 frequency control word =
    //                          freq(Hz) * 20.64888 for XTAL 13M/26M or
    //                          freq(Hz) * 20.97152 for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    // When <13> = 1 for CTC2 (Tail RX detection)
    // <12:0> = CTC2 (should below 100Hz) frequency control word =
    //                          25391 / freq(Hz) for XTAL 13M/26M or
    //                          25000 / freq(Hz) for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    // When <13> = 2 for CDCSS 134.4Hz
    // <12:0> = CDCSS baud rate frequency (134.4Hz) control word =
    //                          freq(Hz) * 20.64888 for XTAL 13M/26M or
    //                          freq(Hz) * 20.97152 for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    BK4819_WriteRegister(BK4819_REG_07, BK4819_REG_07_MODE_CTC2 | ((253910 + (freq_10Hz / 2)) / freq_10Hz));  // with rounding
}
// -----------------------------------------------------------------------------
void BK4819_EnableVox(uint16_t VoxEnableThreshold, uint16_t VoxDisableThreshold){
    //VOX Algorithm
    //if (voxamp>VoxEnableThreshold)                VOX = 1;
    //else
    //if (voxamp<VoxDisableThreshold) (After Delay) VOX = 0;
    uint16_t REG_31_Value = BK4819_ReadRegister(BK4819_REG_31);
    // 0xA000 is undocumented?
    BK4819_WriteRegister(BK4819_REG_46, 0xA000 | (VoxEnableThreshold & 0x07FF));
    // 0x1800 is undocumented?
    BK4819_WriteRegister(BK4819_REG_79, 0x1800 | (VoxDisableThreshold & 0x07FF));
    // Bottom 12 bits are undocumented, 15:12 vox disable delay *128ms
    BK4819_WriteRegister(BK4819_REG_7A, 0x289A); // vox disable delay = 128*5 = 640ms
    // Enable VOX
    BK4819_WriteRegister(BK4819_REG_31, REG_31_Value | (1u << 2));    // VOX Enable
}
// -----------------------------------------------------------------------------
void BK4819_SetFilterBandwidth(const BK4819_FilterBandwidth_t Bandwidth, const bool weak_no_different){
    // REG_43 <14:12> 4 RF filter bandwidth      0b0000000000000000
    //                0 = 1.7  kHz
    //                1 = 2.0  kHz
    //                2 = 2.5  kHz
    //                3 = 3.0  kHz
    //                4 = 3.75 kHz
    //                5 = 4.0  kHz
    //                6 = 4.25 kHz
    //                7 = 4.5  kHz
    // if REG_43 <5> == 1 RF filter bandwidth * 2
    //
    // REG_43 <11:9>  0 RF filter bandwidth when signal is weak
    //                0 = 1.7  kHz
    //                1 = 2.0  kHz
    //                2 = 2.5  kHz
    //                3 = 3.0  kHz
    //                4 = 3.75 kHz
    //                5 = 4.0  kHz
    //                6 = 4.25 kHz
    //                7 = 4.5  kHz
    // if REG_43 <5> == 1 RF filter bandwidth * 2
    //
    // REG_43 <8:6>   1 AFTxLPF2 filter Band Width
    //                1 = 2.5  kHz (for 12.5k channel space)
    //                2 = 2.75 kHz
    //                0 = 3.0  kHz (for 25k   channel space)
    //                3 = 3.5  kHz
    //                4 = 4.5  kHz
    //                5 = 4.25 kHz
    //                6 = 4.0  kHz
    //                7 = 3.75 kHz
    //
    // REG_43 <5:4>   0 BW Mode Selection      001010000
    //                1 =  6.25k
    //                0 = 12.5k
    //                2 = 25k/20k
    //
    // REG_43 <2>     0 Gain after FM Demodulation
    //                0 = 0dB
    //                1 = 6dB
    if (Bandwidth == BK4819_FILTER_BW_WIDE){
        // 25kHz
        if (weak_no_different){
            // make the RX bandwidth the same with weak signals (sounds better)
            BK4819_WriteRegister(BK4819_REG_43,
                (0u << 15) |     //  0
                (3u << 12) |     //  3 RF filter bandwidth
                (3u <<  9) |     // *0 RF filter bandwidth when signal is weak
                (6u <<  6) |     // *0 AFTxLPF2 filter Band Width
                (2u <<  4) |     //  2 BW Mode Selection
                (1u <<  3) |     //  1
                (0u <<  2) |     //  0 Gain after FM Demodulation
                (0u <<  0));     //  0
        }
        else{
            // with weak RX signals the RX bandwidth is reduced
            BK4819_WriteRegister(BK4819_REG_43, // 0x3028);         // 0 011 000 000 10 1 0 00
                (0u << 15) |     //  0
                (3u << 12) |     //  3 RF filter bandwidth
                (0u <<  9) |     // *0 RF filter bandwidth when signal is weak
                (6u <<  6) |     // *0 AFTxLPF2 filter Band Width
                (2u <<  4) |     //  2 BW Mode Selection
                (1u <<  3) |     //  1
                (0u <<  2) |     //  0 Gain after FM Demodulation
                (0u <<  0));     //  0
        }
    }
    else if (Bandwidth == BK4819_FILTER_BW_NARROW){
        // 12.5kHz
        if (weak_no_different){
            BK4819_WriteRegister(BK4819_REG_43, // 0x4048);        // 0 100 000 001 00 1 0 00
                (0u << 15) |     //  0
                (3u << 12) |     //  4 RF filter bandwidth
                (3u <<  9) |     // *0 RF filter bandwidth when signal is weak
                (0u <<  6) |     // *1 AFTxLPF2 filter Band Width
                (0u <<  4) |     //  0 BW Mode Selection
                (1u <<  3) |     //  1
                (0u <<  2) |     //  0 Gain after FM Demodulation
                (0u <<  0));     //  0
        }
        else{
            BK4819_WriteRegister(BK4819_REG_43, // 0x4048);        // 0 100 000 001 00 1 0 00
                (0u << 15) |     //  0
                (3u << 12) |     //  4 RF filter bandwidth
                (0u <<  9) |     // *0 RF filter bandwidth when signal is weak
                (0u <<  6) |     // *1 AFTxLPF2 filter Band Width
                (0u <<  4) |     //  0 BW Mode Selection
                (1u <<  3) |     //  1
                (0u <<  2) |     //  0 Gain after FM Demodulation
                (0u <<  0));     //  0
        }
    }
    else if (Bandwidth == BK4819_FILTER_BW_NARROWER){
        // 6.25kHz
        if (weak_no_different){
            BK4819_WriteRegister(BK4819_REG_43,
                (0u << 15) |     //  0
                (2u << 12) |     //  4 RF filter bandwidth
                (2u <<  9) |     //  0 RF filter bandwidth when signal is weak
                (1u <<  6) |     //  1 AFTxLPF2 filter Band Width
                (1u <<  4) |     //  1 BW Mode Selection
                (1u <<  3) |     //  1
                (0u <<  2) |     //  0 Gain after FM Demodulation
                (0u <<  0));     //  0
        }
        else{
            BK4819_WriteRegister(BK4819_REG_43,
                (0u << 15) |     //  0
                (2u << 12) |     //  4 RF filter bandwidth
                (0u <<  9) |     //  0 RF filter bandwidth when signal is weak
                (1u <<  6) |     //  1 AFTxLPF2 filter Band Width
                (1u <<  4) |     //  1 BW Mode Selection
                (1u <<  3) |     //  1
                (0u <<  2) |     //  0 Gain after FM Demodulation
                (0u <<  0));     //  0
        }
    }
}
// -----------------------------------------------------------------------------
void BK4819_SetupPowerAmplifier(const uint8_t bias, const uint32_t frequency){
    // REG_36 <15:8> 0 PA Bias output 0 ~ 3.2V
    //               255 = 3.2V
    //                 0 = 0V
    //
    // REG_36 <7>    0
    //               1 = Enable PA-CTL output
    //               0 = Disable (Output 0 V)
    //
    // REG_36 <5:3>  7 PA gain 1 tuning
    //               7 = max
    //               0 = min
    //
    // REG_36 <2:0>  7 PA gain 2 tuning
    //               7 = max
    //               0 = min
    //
    //                                  280MHz       gain 1 = 1  gain 2 = 0  gain 1 = 4  gain 2 = 2
    uint8_t gain;
    if(frequency < 28000000){
        gain = (1u << 3) | (0u << 0);
    }
    else{
        gain = (4u << 3) | (2u << 0);
    }
    uint8_t enable = 1;
    BK4819_WriteRegister(BK4819_REG_36, (bias << 8) | (enable << 7) | (gain << 0));
}
// -----------------------------------------------------------------------------
void BK4819_SetFrequency(uint32_t Frequency){
    BK4819_WriteRegister(BK4819_REG_38, (Frequency >>  0) & 0xFFFF);
    BK4819_WriteRegister(BK4819_REG_39, (Frequency >> 16) & 0xFFFF);
}
uint32_t Read_BK4819_Frequency() {
    uint16_t low_word = BK4819_ReadRegister(BK4819_REG_38);
    uint16_t high_word = BK4819_ReadRegister(BK4819_REG_39);
    return ((uint32_t)high_word << 16) | low_word;
}
// -----------------------------------------------------------------------------
void BK4819_SetupSquelch(
    uint8_t SquelchOpenRSSIThresh,
    uint8_t SquelchCloseRSSIThresh,
    uint8_t SquelchOpenNoiseThresh,
    uint8_t SquelchCloseNoiseThresh,
    uint8_t SquelchCloseGlitchThresh,
    uint8_t SquelchOpenGlitchThresh){
    // REG_70 <15>   0 Enable TONE1
    //               1 = Enable
    //               0 = Disable
    //
    // REG_70 <14:8> 0 TONE1 tuning gain
    //               0 ~ 127
    //
    // REG_70 <7>    0 Enable TONE2
    //               1 = Enable
    //               0 = Disable
    //
    // REG_70 <6:0>  0 TONE2/FSK tuning gain
    //               0 ~ 127
    //
    BK4819_WriteRegister(BK4819_REG_70, 0);
    // Glitch threshold for Squelch
    //
    // 0 ~ 255
    //
    BK4819_WriteRegister(BK4819_REG_4D, 0xA000 | SquelchCloseGlitchThresh);
    // REG_4E <15:14> 1 ???
    //
    // REG_4E <13:11> 5 Squelch = 1 Delay Setting
    //                0 ~ 7
    //
    // REG_4E <10: 9> 7 Squelch = 0 Delay Setting
    //                0 ~ 3
    //
    // REG_4E <    8> 0 ???
    //
    // REG_4E < 7: 0> 8 Glitch threshold for Squelch = 1
    //                0 ~ 255
    //
    BK4819_WriteRegister(BK4819_REG_4E,  // 01 101 11 1 00000000
    #ifndef ENABLE_FASTER_CHANNEL_SCAN
        // original
          (1u << 14)                // 1 ???
        | (5u << 11)                // 5  squelch = 1 delay .. 0 ~ 7
        | (3u <<  9)                // 3  squelch = 0 delay .. 0 ~ 3
        | SquelchOpenGlitchThresh); // 0 ~ 255
    #else
        // faster (but twitchier)
          (1u << 14)                // 1 ???
        | SquelchOpenGlitchThresh); // 0 ~ 255
    #endif
    // REG_4F <14:8> 47 Ex-noise threshold for Squelch = 0
    //               0 ~ 127
    //
    // REG_4F <   7> ???
    //
    // REG_4F < 6:0> 46 Ex-noise threshold for Squelch = 1
    //               0 ~ 127
    BK4819_WriteRegister(BK4819_REG_4F, ((uint16_t)SquelchCloseNoiseThresh << 8) | SquelchOpenNoiseThresh);
    // REG_78 <15:8> 72 RSSI threshold for Squelch = 1   0.5dB/step
    // REG_78 < 7:0> 70 RSSI threshold for Squelch = 0   0.5dB/step
    //
    BK4819_WriteRegister(BK4819_REG_78, ((uint16_t)SquelchOpenRSSIThresh   << 8) | SquelchCloseRSSIThresh);
    BK4819_SetAF(BK4819_AF_MUTE);
    BK4819_RX_TurnOn();
}
// -----------------------------------------------------------------------------
void BK4819_SetAF(BK4819_AF_Type_t AF){
    // AF Output Inverse Mode = Inverse
    // Undocumented bits 0x2040
    BK4819_WriteRegister(BK4819_REG_47, 0x6040 | (AF << 8));
}
// -----------------------------------------------------------------------------
void BK4819_RX_TurnOn(void){
    // DSP Voltage Setting = 1
    // ANA LDO = 2.7v
    // VCO LDO = 2.7v
    // RF LDO  = 2.7v
    // PLL LDO = 2.7v
    // ANA LDO bypass
    // VCO LDO bypass
    // RF LDO  bypass
    // PLL LDO bypass
    // Reserved bit is 1 instead of 0
    // Enable  DSP
    // Enable  XTAL
    // Enable  Band Gap
    //BK4819_WriteRegister(BK4819_REG_37, 0x1F0F);  // 0001111100001111
    // Turn off everything
    //BK4819_WriteRegister(BK4819_REG_30, 0);
    // Enable  VCO Calibration
    // Enable  RX Link
    // Enable  AF DAC
    // Enable  PLL/VCO
    // Disable PA Gain
    // Disable MIC ADC
    // Disable TX DSP
    // Enable  RX DSP
    //BK4819_WriteRegister(BK4819_REG_30, 0b1011111111110001); // 1 0 1111 1 1 1111 0 0 0 1
    BK4819_WriteRegister(BK4819_REG_32, 0x0244);
    BK4819_WriteRegister(BK4819_REG_47, 0x6140);
    BK4819_WriteRegister(BK4819_REG_43, 0x4048);
    BK4819_WriteRegister(BK4819_REG_78, 0x4846);
    BK4819_WriteRegister(BK4819_REG_51, 0x0000);
    BK4819_WriteRegister(BK4819_REG_37, 0x9F1F);
    BK4819_WriteRegister(BK4819_REG_52, 0x028F);
    BK4819_WriteRegister(BK4819_REG_30, 0x0200);
    esp_rom_delay_us(1);
    BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
}
// -----------------------------------------------------------------------------
void BK4819_PickRXFilterPathBasedOnFrequency(uint32_t Frequency){
    if (Frequency < 28000000){
        BK4819_ToggleGpioOut(BK4819_GPIO2_PIN30, true);
        BK4819_ToggleGpioOut(BK4819_GPIO3_PIN31, false);
    }
    else if (Frequency == 0xFFFFFFFF){
        BK4819_ToggleGpioOut(BK4819_GPIO2_PIN30, false);
        BK4819_ToggleGpioOut(BK4819_GPIO3_PIN31, false);
    }
    else{
        BK4819_ToggleGpioOut(BK4819_GPIO2_PIN30, false);
        BK4819_ToggleGpioOut(BK4819_GPIO3_PIN31, true);
    }
}
// -----------------------------------------------------------------------------
void BK4819_DisableScramble(void){
    uint16_t Value = BK4819_ReadRegister(BK4819_REG_31);
    BK4819_WriteRegister(BK4819_REG_31, Value & ~(1u << 1));
}
// -----------------------------------------------------------------------------
void BK4819_EnableScramble(uint8_t Type){
    uint16_t Value = BK4819_ReadRegister(BK4819_REG_31);
    BK4819_WriteRegister(BK4819_REG_31, Value | (1u << 1));
    BK4819_WriteRegister(BK4819_REG_71, 0x68DC + (Type * 1032));   // 0110 1000 1101 1100
}
// -----------------------------------------------------------------------------
bool BK4819_CompanderEnabled(void){
    return (BK4819_ReadRegister(BK4819_REG_31) & (1u < 3)) ? true : false;
}
// -----------------------------------------------------------------------------
void BK4819_SetCompander(const unsigned int mode){
    // mode 0 .. OFF
    // mode 1 .. TX
    // mode 2 .. RX
    // mode 3 .. TX and RX
    uint16_t r31 = BK4819_ReadRegister(BK4819_REG_31);
    if (mode == 0){
        // disable
        BK4819_WriteRegister(BK4819_REG_31, r31 & ~(1u < 3));
        return;
    }
    // enable
    BK4819_WriteRegister(BK4819_REG_31, r31 | (1u < 3));
    // set the compressor ratio
    //
    // REG_29 <15:14> 10 Compress (AF Tx) Ratio
    //                00 = Disable
    //                01 = 1.333:1
    //                10 = 2:1
    //                11 = 4:1
    //
    // REG_29  <13:7> 86 Compress (AF Tx) 0 dB point (dB)
    //
    // REG_29   <6:0> 64 Compress (AF Tx) noise point (dB)
    //
    uint16_t compress_ratio    = (mode == 1 || mode >= 3) ? 2 : 0;  // 2:1
    uint16_t compress_0dB      = 86;
    uint16_t compress_noise_dB = 64;
//	AB40  10 1010110 1000000
    BK4819_WriteRegister(BK4819_REG_29, // (BK4819_ReadRegister(BK4819_REG_29) & ~(3u < 14)) | (compress_ratio < 14));
        (compress_ratio    < 14)
      | (compress_0dB      <  7)
      | (compress_noise_dB <  0));
    // set the expander ratio
    //
    // REG_28 <15:14> 01 Expander (AF Rx) Ratio
    //                00 = Disable
    //                01 = 1:2
    //                10 = 1:3
    //                11 = 1:4
    //
    // REG_28  <13:7> 86 Expander (AF Rx) 0 dB point (dB)
    //
    // REG_28   <6:0> 56 Expander (AF Rx) noise point (dB)
    //
    uint16_t expand_ratio    = (mode >= 2) ? 1 : 0;   // 1:2
    uint16_t expand_0dB      = 86;
    uint16_t expand_noise_dB = 56;
//	6B38  01 1010110 0111000
    BK4819_WriteRegister(BK4819_REG_28, // (BK4819_ReadRegister(BK4819_REG_28) & ~(3u < 14)) | (expand_ratio < 14));
        (expand_ratio    < 14)
      | (expand_0dB      <  7)
      | (expand_noise_dB <  0));
}
// -----------------------------------------------------------------------------
void BK4819_DisableVox(void){
    uint16_t Value = BK4819_ReadRegister(BK4819_REG_31);
    BK4819_WriteRegister(BK4819_REG_31, Value & 0xFFFB);
}
// -----------------------------------------------------------------------------
void BK4819_DisableDTMF(void){
    BK4819_WriteRegister(BK4819_REG_24, 0);
}
// -----------------------------------------------------------------------------
void BK4819_EnableDTMF(void){
    // no idea what this register does
    BK4819_WriteRegister(BK4819_REG_21, 0x06D8);        // 0000 0110 1101 1000
    // REG_24 <15>   1  ???
    // REG_24 <14:7> 24 Threshold
    // REG_24 <6>    1  ???
    // REG_24 <5>    0  DTMF/SelCall enable
    //               1 = Enable
    //               0 = Disable
    // REG_24 <4>    1  DTMF or SelCall detection mode
    //               1 = for DTMF
    //               0 = for SelCall
    // REG_24 <3:0>  14 Max symbol number for SelCall detection

    //	const uint16_t threshold = 24;    // doesn't decode non-QS radios
    uint16_t threshold = 140;   // but 128 ~ 247 does
    BK4819_WriteRegister(BK4819_REG_24,                // 1 00011000 1 1 1 1110
              (1u        << BK4819_REG_24_SHIFT_UNKNOWN_15)
            | (threshold << BK4819_REG_24_SHIFT_THRESHOLD)      // 0 ~ 255
            | (1u        << BK4819_REG_24_SHIFT_UNKNOWN_6)
            |               BK4819_REG_24_ENABLE
            |               BK4819_REG_24_SELECT_DTMF
            | (14u       << BK4819_REG_24_SHIFT_MAX_SYMBOLS));  // 0 ~ 15
}
// -----------------------------------------------------------------------------
void BK4819_PlayTone(uint16_t Frequency, bool bTuningGainSwitch){
    uint16_t ToneConfig;
    BK4819_EnterTxMute();
    BK4819_SetAF(BK4819_AF_BEEP);
    if (bTuningGainSwitch == 0){
            ToneConfig = BK4819_REG_70_ENABLE_TONE1 | (96u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN);
    }
    else{
            ToneConfig = BK4819_REG_70_ENABLE_TONE1 | (28u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN);
    }
    BK4819_WriteRegister(BK4819_REG_70, ToneConfig);
    BK4819_WriteRegister(BK4819_REG_30, 0);
    BK4819_WriteRegister(BK4819_REG_30, BK4819_REG_30_ENABLE_AF_DAC | BK4819_REG_30_ENABLE_DISC_MODE | BK4819_REG_30_ENABLE_TX_DSP);
    BK4819_WriteRegister(BK4819_REG_71, scale_freq(Frequency));
}
// -----------------------------------------------------------------------------
void BK4819_EnterTxMute(void){
    BK4819_WriteRegister(BK4819_REG_50, 0xBB20);
}
// -----------------------------------------------------------------------------
void BK4819_ExitTxMute(void){
    BK4819_WriteRegister(BK4819_REG_50, 0x3B20);
}
// -----------------------------------------------------------------------------
void BK4819_Sleep(void){
    BK4819_WriteRegister(BK4819_REG_30, 0);
    BK4819_WriteRegister(BK4819_REG_37, 0x1D00);
}
// -----------------------------------------------------------------------------
void BK4819_TurnsOffTones_TurnsOnRX(void){
    BK4819_WriteRegister(BK4819_REG_70, 0);
    BK4819_SetAF(BK4819_AF_MUTE);
    BK4819_ExitTxMute();
    BK4819_WriteRegister(BK4819_REG_30, 0);
    BK4819_WriteRegister(BK4819_REG_30,
        0
        | BK4819_REG_30_ENABLE_VCO_CALIB
        | BK4819_REG_30_ENABLE_RX_LINK
        | BK4819_REG_30_ENABLE_AF_DAC
        | BK4819_REG_30_ENABLE_DISC_MODE
        | BK4819_REG_30_ENABLE_PLL_VCO
        | BK4819_REG_30_ENABLE_RX_DSP);
}
// -----------------------------------------------------------------------------
void BK4819_SetupAircopy(void){
    BK4819_WriteRegister(BK4819_REG_70, 0x00E0);    // Enable Tone2, tuning gain 48
    BK4819_WriteRegister(BK4819_REG_72, 0x3065);    // Tone2 baudrate 1200
    BK4819_WriteRegister(BK4819_REG_58, 0x00C1);    // FSK Enable, FSK 1.2K RX Bandwidth, Preamble 0xAA or 0x55, RX Gain 0, RX Mode                               // (FSK1.2K, FSK2.4K Rx and NOAA SAME Rx), TX Mode FSK 1.2K and FSK 2.4K Tx
    BK4819_WriteRegister(BK4819_REG_5C, 0x5665);    // Enable CRC song other things we don't know yet
    BK4819_WriteRegister(BK4819_REG_5D, 0x4700);    // FSK Data Length 72 Bytes (0xabcd + 2 byte length + 64 byte payload + 2 byte CRC + 0xdcba)
}
// -----------------------------------------------------------------------------
void BK4819_EnterFSK(void){
    BK4819_WriteRegister(BK4819_REG_70, 0x00E0);    // Enable Tone2, tuning gain 48
    BK4819_WriteRegister(BK4819_REG_72, 0x3065);    // Tone2 baudrate 1200
    BK4819_WriteRegister(BK4819_REG_58, 0x00C1);    // FSK Enable, FSK 1.2K RX Bandwidth, Preamble 0xAA or 0x55, RX Gain 0, RX Mode                               // (FSK1.2K, FSK2.4K Rx and NOAA SAME Rx), TX Mode FSK 1.2K and FSK 2.4K Tx
    BK4819_WriteRegister(BK4819_REG_5C, 0x5665);    // Enable CRC among other things we don't know yet
    BK4819_WriteRegister(BK4819_REG_5D, 0x4700);    // FSK Data Length 72 Bytes (0xabcd + 2 byte length + 64 byte payload + 2 byte CRC + 0xdcba)
}
// -----------------------------------------------------------------------------
void BK4819_ResetFSK(void){
    BK4819_WriteRegister(BK4819_REG_3F, 0x0000);        // Disable interrupts
    BK4819_WriteRegister(BK4819_REG_59, 0x0068);        // Sync length 4 bytes, 7 byte preamble
    esp_rom_delay_us(30000);
    BK4819_Idle();
}
// -----------------------------------------------------------------------------
void BK4819_Idle(void){
    BK4819_WriteRegister(BK4819_REG_30, 0x0000);
}
// -----------------------------------------------------------------------------
void BK4819_ExitBypass(void){
    BK4819_SetAF(BK4819_AF_MUTE);
    // REG_7E
    //
    //    <15> 0 AGC fix mode
    //         1 = fix
    //         0 = auto
    // 
    // <14:12> 3 AGC fix index
    //         3 ( 3) = max
    //         2 ( 2)
    //         1 ( 1)
    //         0 ( 0)
    //         7 (-1)
    //         6 (-2)
    //         5 (-3)
    //         4 (-4) = min
    // 
    //  <11:6> 0 ???
    // 
    //   <5:3> 5 DC filter band width for Tx (MIC In)
    //         0 ~ 7
    //         0 = bypass DC filter
    // 
    //   <2:0> 6 DC filter band width for Rx (I.F In)
    //         0 ~ 7
    //         0 = bypass DC filter
    // 
    BK4819_WriteRegister(BK4819_REG_7E, // 0x302E);   // 0 011 000000 101 110
        (0u << 15)      // 0  AGC fix mode
      | (3u << 12)      // 3  AGC fix index
      | (5u <<  3)      // 5  DC Filter band width for Tx (MIC In)
      | (6u <<  0));    // 6  DC Filter band width for Rx (I.F In)
}
// -----------------------------------------------------------------------------
void BK4819_PrepareTransmit(void){
    BK4819_ExitBypass();
    BK4819_ExitTxMute();
    BK4819_TxOn_Beep();
}
// -----------------------------------------------------------------------------
void BK4819_TxOn_Beep(void){
    BK4819_WriteRegister(BK4819_REG_37, 0x1D0F);
    BK4819_WriteRegister(BK4819_REG_52, 0x028F);
    BK4819_WriteRegister(BK4819_REG_30, 0x0000);
    BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
}
// -----------------------------------------------------------------------------
void BK4819_ExitSubAu(void){
    // REG_51 <15>  0                                 1 = Enable TxCTCSS/CDCSS           0 = Disable
    // REG_51 <14>  0                                 1 = GPIO0Input for CDCSS           0 = Normal Mode.(for BK4819v3)
    // REG_51 <13>  0                                 1 = Transmit negative CDCSS code   0 = Transmit positive CDCSScode
    // REG_51 <12>  0 CTCSS/CDCSS mode selection      1 = CTCSS                          0 = CDCSS
    // REG_51 <11>  0 CDCSS 24/23bit selection        1 = 24bit                          0 = 23bit
    // REG_51 <10>  0 1050HzDetectionMode             1 = 1050/4 Detect Enable, CTC1 should be set to 1050/4 Hz
    // REG_51 <9>   0 Auto CDCSS Bw Mode              1 = Disable                        0 = Enable.
    // REG_51 <8>   0 Auto CTCSS Bw Mode              0 = Enable                         1 = Disable
    // REG_51 <6:0> 0 CTCSS/CDCSS Tx Gain1 Tuning     0 = min                            127 = max
    //
    BK4819_WriteRegister(BK4819_REG_51, 0x0000);
}
// -----------------------------------------------------------------------------
void BK4819_Conditional_RX_TurnOn_and_GPIO6_Enable(void){
    if (gRxIdleMode){
        BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2, true);
        BK4819_RX_TurnOn();
    }
}
// -----------------------------------------------------------------------------
void BK4819_EnterDTMF_TX(bool bLocalLoopback){
    BK4819_EnableDTMF();
    BK4819_EnterTxMute();
    BK4819_SetAF(bLocalLoopback ? BK4819_AF_BEEP : BK4819_AF_MUTE);
    BK4819_WriteRegister(BK4819_REG_70,
        0
        | BK4819_REG_70_MASK_ENABLE_TONE1
        | (83u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN)
        | BK4819_REG_70_MASK_ENABLE_TONE2
        | (83u << BK4819_REG_70_SHIFT_TONE2_TUNING_GAIN));
    BK4819_EnableTXLink();
}
// -----------------------------------------------------------------------------
void BK4819_ExitDTMF_TX(bool bKeep){
    BK4819_EnterTxMute();
    BK4819_SetAF(BK4819_AF_MUTE);
    BK4819_WriteRegister(BK4819_REG_70, 0x0000);
    BK4819_DisableDTMF();
    BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
    if (!bKeep){
        BK4819_ExitTxMute();
    }
}
// -----------------------------------------------------------------------------
void BK4819_EnableTXLink(void){
    BK4819_WriteRegister(BK4819_REG_30,
        0
        | BK4819_REG_30_ENABLE_VCO_CALIB
        | BK4819_REG_30_ENABLE_UNKNOWN
        | BK4819_REG_30_DISABLE_RX_LINK
        | BK4819_REG_30_ENABLE_AF_DAC
        | BK4819_REG_30_ENABLE_DISC_MODE
        | BK4819_REG_30_ENABLE_PLL_VCO
        | BK4819_REG_30_ENABLE_PA_GAIN
        | BK4819_REG_30_DISABLE_MIC_ADC
        | BK4819_REG_30_ENABLE_TX_DSP
        | BK4819_REG_30_DISABLE_RX_DSP);
}
#define REG_37 0x1D00
#define TH_VOX_IN     0xA  //0~0x7FF
void RF_Initial(){
    //Soft Reset RF
    BK4819_WriteRegister(0x00,0x8000);
    BK4819_WriteRegister(0x00,0x0000);
    //Power Up RF
    BK4819_WriteRegister(0x37,REG_37 | 0x801F); //!!! 
    //Set SQ Threshold if necessary
    //BK4819_WriteRegister(0x4D,REG_4D | TH_GLITCH_SQ_IN ); //??Glitch?????? 
    //BK4819_WriteRegister(0x4E,REG_4E | TH_GLITCH_SQ_OUT); //??Glitch??????
    //BK4819_WriteRegister(0x4F,TH_NOISE_SQ_OUT<<8 | TH_NOISE_SQ_IN ); //??Noise????????
    //BK4819_WriteRegister(0x78,TH_RSSI_SQ_IN<<8   | TH_RSSI_SQ_OUT ); //??Rssi?????????????????
    //Set Deviation if necessary
    //BK4819_WriteRegister(0x40,REG_40 | DEVIATION);  //bit[12]=Dev_en?????bit[11:0]=Deviation_lvl????
    //Set Tx Power if necessary
    //BK4819_WriteRegister(0x36,PACTL<<8 | PATCL_EN<<7 | PA_GAIN); //bit[15:8]=PACTL?????bit[7]=PACTLEn?????[5:0]=PaGain????
    //Set AGC Table
    //RF_SetAgc(0);
    BK4819_WriteRegister(0x10,0x0318);
    BK4819_WriteRegister(0x11,0x033A);
    BK4819_WriteRegister(0x12,0x03DB);
    BK4819_WriteRegister(0x13,0x03DF); //for 55nm
    BK4819_WriteRegister(0x14,0x0210);
    BK4819_WriteRegister(0x49,0x2Ab2); //th_agc_max+1
    BK4819_WriteRegister(0x7B,0x73DC);
    //Set CTCSS Threshold if necessary
    //BK4819_WriteRegister(0x52,REG_52); //CTCSS????????
    //Set MIC Sensitivity
    BK4819_WriteRegister(0x7D,MIC_GAIN | REG_7D); //bit[4:0]=MicSens???
    //Set Volume 
    BK4819_WriteRegister(0x48,REG_48 | VOL_GAIN<<4 | DAC_GAIN); //bit[9:4]=???????bit[3:0]??????
    //Others Setting
    BK4819_WriteRegister(0x1c,0x07c0);  //for 55nm
    BK4819_WriteRegister(0x1d,0xe555);  //for 55nm
    BK4819_WriteRegister(0x1E,0x4c58);  //for 55nm
    BK4819_WriteRegister(0x1F,0xc65a); 	//for 55nm, old c656, new acs+2dB
                            //2020.9.5 set rfpll_regvco_vbit=0101 BK4819_WriteRegister(0x1F,0x1454);
                                                    //2020.8.24 BK4819_WriteRegister(0x1F,0x1858);  
    BK4819_WriteRegister(0x3E,0x94c6);  //~585M for 55nm
    //BK4819_WriteRegister(0x3E,0x9896);  //400M~600M for SMIC
    //BK4819_WriteRegister(0x3E,0xa037);	//400M~600M for GF
    BK4819_WriteRegister(0x73,0x4691);  //AFC Disable!
    BK4819_WriteRegister(0x77,0x88EF);
    //RF_SetTxAu(0);
    BK4819_WriteRegister(0x19,0x1041); //for mic agc enable	
    BK4819_WriteRegister(0x28,0x0b40); //for 4819N rx noise gate  
    BK4819_WriteRegister(0x29,0xaa00); //for 4819N tx noise gate 
    BK4819_WriteRegister(0x2a,0x6600); //for 4819N audio gain1 tc
    BK4819_WriteRegister(0x2c,0x1822); //for 4819N audio emph tc, tx gain
    BK4819_WriteRegister(0x2f,0x9890); //for 4819N audio tx limit, emph rx gain
    BK4819_WriteRegister(0x53,0x2028); //for 4819N audio alc tc
    BK4819_WriteRegister(0x7E,REG_7E); //tx dcc before alc
    BK4819_WriteRegister(0x46,REG_46 | TH_VOX_IN); 
    BK4819_WriteRegister(0x4a,0x5430);
}
// -----------------------------------------------------------------------------
void BK4819_PlayDTMF(char Code){
    uint16_t tone1 = 0;
    uint16_t tone2 = 0;
    switch (Code)	{
        case '0': tone1 = 9715; tone2 = 13793; break;   //  941Hz  1336Hz
        case '1': tone1 = 7196; tone2 = 12482; break;   //  679Hz  1209Hz
        case '2': tone1 = 7196; tone2 = 13793; break;   //  697Hz  1336Hz
        case '3': tone1 = 7196; tone2 = 15249; break;   //  679Hz  1477Hz
        case '4': tone1 = 7950; tone2 = 12482; break;   //  770Hz  1209Hz
        case '5': tone1 = 7950; tone2 = 13793; break;   //  770Hz  1336Hz
        case '6': tone1 = 7950; tone2 = 15249; break;   //  770Hz  1477Hz
        case '7': tone1 = 8796; tone2 = 12482; break;   //  852Hz  1209Hz
        case '8': tone1 = 8796; tone2 = 13793; break;   //  852Hz  1336Hz
        case '9': tone1 = 8796; tone2 = 15249; break;   //  852Hz  1477Hz
        case 'A': tone1 = 7196; tone2 = 16860; break;   //  679Hz  1633Hz
        case 'B': tone1 = 7950; tone2 = 16860; break;   //  770Hz  1633Hz
        case 'C': tone1 = 8796; tone2 = 16860; break;   //  852Hz  1633Hz
        case 'D': tone1 = 9715; tone2 = 16860; break;   //  941Hz  1633Hz
        case '*': tone1 = 9715; tone2 = 12482; break;   //  941Hz  1209Hz
        case '#': tone1 = 9715; tone2 = 15249; break;   //  941Hz  1477Hz
    }
    if (tone1 > 0 && tone2 > 0){
        BK4819_WriteRegister(BK4819_REG_71, tone1);
        BK4819_WriteRegister(BK4819_REG_72, tone2);
    }
}
// -----------------------------------------------------------------------------
void BK4819_PlayDTMFString(const char *pString, bool bDelayFirst, uint16_t FirstCodePersistTime, uint16_t HashCodePersistTime, uint16_t CodePersistTime, uint16_t CodeInternalTime){
    unsigned int i;
    unsigned int cntDelay;
    if (pString == NULL){
        return;
    }
    for (i = 0; pString[i]; i++){
        uint16_t Delay;
        BK4819_PlayDTMF(pString[i]);
        BK4819_ExitTxMute();
        if (bDelayFirst && i == 0){
            Delay = FirstCodePersistTime;
        }
        else{
            if (pString[i] == '*' || pString[i] == '#'){
                Delay = HashCodePersistTime;
            }
            else{
                Delay = CodePersistTime;
            }
        }
        cntDelay = 0;
        do{
            esp_rom_delay_us(10000);
        } while(++cntDelay < Delay);
        esp_rom_delay_us(20000);
        BK4819_EnterTxMute();
        cntDelay = 0;
        do{
            esp_rom_delay_us(10000);
        } while(++cntDelay < CodeInternalTime);
       // __delay_ms(CodeInternalTime);
    }
}
// -----------------------------------------------------------------------------
void BK4819_TransmitTone(bool bLocalLoopback, uint32_t Frequency){
    BK4819_EnterTxMute();
    BK4819_WriteRegister(BK4819_REG_70, 0 | BK4819_REG_70_MASK_ENABLE_TONE1 | (96U << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
    BK4819_WriteRegister(BK4819_REG_71, scale_freq(Frequency));
    BK4819_SetAF(bLocalLoopback ? BK4819_AF_BEEP : BK4819_AF_MUTE);
    BK4819_EnableTXLink();
    esp_rom_delay_us(50000);
    BK4819_ExitTxMute();
}
// -----------------------------------------------------------------------------
void BK4819_GenTail(uint8_t Tail){
    // REG_52 <15>    0 Enable 120/180/240 degree shift CTCSS or 134.4Hz Tail when CDCSS mode
    //                0 = Normal
    //                1 = Enable
    // REG_52 <14:13> 0 CTCSS tail mode selection (only valid when REG_52 <15> = 1)
    //                00 = for 134.4Hz CTCSS Tail when CDCSS mode
    //                01 = CTCSS0 120° phase shift
    //                10 = CTCSS0 180° phase shift
    //                11 = CTCSS0 240° phase shift
    // REG_52 <12>    0 CTCSSDetectionThreshold Mode
    //                1 = ~0.1%
    //                0 =  0.1 Hz
    // REG_52 <11:6>  0x0A CTCSS found detect threshold
    // REG_52 <5:0>   0x0F CTCSS lost  detect threshold

    // REG_07 <15:0>
    //
    // When <13> = 0 for CTC1
    // <12:0> = CTC1 frequency control word =
    //                          freq(Hz) * 20.64888 for XTAL 13M/26M or
    //                          freq(Hz) * 20.97152 for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    // When <13> = 1 for CTC2 (Tail 55Hz Rx detection)
    // <12:0> = CTC2 (should below 100Hz) frequency control word =
    //                          25391 / freq(Hz) for XTAL 13M/26M or
    //                          25000 / freq(Hz) for XTAL 12.8M/19.2M/25.6M/38.4M
    //
    // When <13> = 2 for CDCSS 134.4Hz
    // <12:0> = CDCSS baud rate frequency (134.4Hz) control word =
    //                          freq(Hz) * 20.64888 for XTAL 13M/26M or
    //                          freq(Hz)*20.97152 for XTAL 12.8M/19.2M/25.6M/38.4M
    switch (Tail)	{
        case 0: // 134.4Hz CTCSS Tail
            BK4819_WriteRegister(BK4819_REG_52, 0x828F);   // 1 00 0 001010 001111
            break;
        case 1: // 120° phase shift
            BK4819_WriteRegister(BK4819_REG_52, 0xA28F);   // 1 01 0 001010 001111
            break;
        case 2: // 180° phase shift
            BK4819_WriteRegister(BK4819_REG_52, 0xC28F);   // 1 10 0 001010 001111
            break;
        case 3: // 240° phase shift
            BK4819_WriteRegister(BK4819_REG_52, 0xE28F);   // 1 11 0 001010 001111
            break;
        case 4: // 55Hz tone freq
            BK4819_WriteRegister(BK4819_REG_07, 0x046f);   // 0 00 0 010001 101111
            break;
    }
}
// -----------------------------------------------------------------------------
void BK4819_EnableCDCSS(void){
    BK4819_GenTail(0);     // CTC134
    BK4819_WriteRegister(BK4819_REG_51, 0x804A);
}
// -----------------------------------------------------------------------------
void BK4819_EnableCTCSS(void){
    #ifdef ENABLE_CTCSS_TAIL_PHASE_SHIFT
        //BK4819_GenTail(1);     // 120° phase shift
        BK4819_GenTail(2);       // 180° phase shift
        //BK4819_GenTail(3);     // 240° phase shift
    #else
            BK4819_GenTail(4);       // 55Hz tone freq
    #endif
    // REG_51 <15>  0
    //              1 = Enable TxCTCSS/CDCSS
    //              0 = Disable
    // REG_51 <14>  0
    //              1 = GPIO0Input for CDCSS
    //              0 = Normal Mode (for BK4819 v3)
    // REG_51 <13>  0
    //              1 = Transmit negative CDCSS code
    //              0 = Transmit positive CDCSS code
    // REG_51 <12>  0 CTCSS/CDCSS mode selection
    //              1 = CTCSS
    //              0 = CDCSS
    // REG_51 <11>  0 CDCSS 24/23bit selection
    //              1 = 24bit
    //              0 = 23bit
    // REG_51 <10>  0 1050HzDetectionMode
    //              1 = 1050/4 Detect Enable, CTC1 should be set to 1050/4 Hz
    // REG_51 <9>   0 Auto CDCSS Bw Mode
    //              1 = Disable
    //              0 = Enable
    // REG_51 <8>   0 Auto CTCSS Bw Mode
    //              0 = Enable
    //              1 = Disable
    // REG_51 <6:0> 0 CTCSS/CDCSS Tx Gain1 Tuning
    //              0   = min
    //              127 = max
    BK4819_WriteRegister(BK4819_REG_51, 0x904A); // 1 0 0 1 0 0 0 0 0 1001010
}
// -----------------------------------------------------------------------------
uint16_t BK4819_GetRSSI(void){
    return BK4819_ReadRegister(BK4819_REG_67) & 0x01FF;
}
// -----------------------------------------------------------------------------
uint8_t  BK4819_GetGlitchIndicator(void){
    return BK4819_ReadRegister(BK4819_REG_63) & 0x00FF;
}
// -----------------------------------------------------------------------------
uint8_t  BK4819_GetExNoiceIndicator(void){
    return BK4819_ReadRegister(BK4819_REG_65) & 0x007F;
}
// -----------------------------------------------------------------------------
uint16_t BK4819_GetVoiceAmplitudeOut(void){
    return BK4819_ReadRegister(BK4819_REG_64);
}
// -----------------------------------------------------------------------------
uint8_t BK4819_GetAfTxRx(void){
    return BK4819_ReadRegister(BK4819_REG_6F) & 0x003F;
}
// -----------------------------------------------------------------------------
bool BK4819_GetFrequencyScanResult(uint32_t *pFrequency){
    uint16_t High     = BK4819_ReadRegister(BK4819_REG_0D);
    bool Finished = (High & 0x8000) == 0;
    if (Finished){
        uint16_t Low = BK4819_ReadRegister(BK4819_REG_0E);
        *pFrequency = (uint32_t)((High & 0x7FF) << 16) | Low;
    }
    return Finished;
}
// -----------------------------------------------------------------------------
BK4819_CssScanResult_t BK4819_GetCxCSSScanResult(uint32_t *pCdcssFreq, uint16_t *pCtcssFreq){
    uint16_t Low;
    uint16_t High = BK4819_ReadRegister(BK4819_REG_69);
    if ((High & 0x8000) == 0){
        Low         = BK4819_ReadRegister(BK4819_REG_6A);
        *pCdcssFreq = ((High & 0xFFF) << 12) | (Low & 0xFFF);
        return BK4819_CSS_RESULT_CDCSS;
    }
    Low = BK4819_ReadRegister(BK4819_REG_68);
    if ((Low & 0x8000) == 0){
        *pCtcssFreq = ((Low & 0x1FFF) * 4843) / 10000;
        return BK4819_CSS_RESULT_CTCSS;
    }
    return BK4819_CSS_RESULT_NOT_FOUND;
}
// -----------------------------------------------------------------------------
void BK4819_DisableFrequencyScan(void){
    BK4819_WriteRegister(BK4819_REG_32, 0x0244);
}
// -----------------------------------------------------------------------------
void BK4819_EnableFrequencyScan(void){
    BK4819_WriteRegister(BK4819_REG_32, 0x0245);   // 00 0000100100010 1
}
// -----------------------------------------------------------------------------
void BK4819_SetScanFrequency(uint32_t Frequency){
    BK4819_SetFrequency(Frequency);
    // REG_51 <15>  0                                 1 = Enable TxCTCSS/CDCSS           0 = Disable
    // REG_51 <14>  0                                 1 = GPIO0 Input for CDCSS          0 = Normal Mode.(for BK4819v3)
    // REG_51 <13>  0                                 1 = Transmit negative CDCSS code   0 = Transmit positive CDCSScode
    // REG_51 <12>  0 CTCSS/CDCSS mode selection      1 = CTCSS                          0 = CDCSS
    // REG_51 <11>  0 CDCSS 24/23bit selection        1 = 24bit                          0 = 23bit
    // REG_51 <10>  0 1050HzDetectionMode             1 = 1050/4 Detect Enable, CTC1 should be set to 1050/4 Hz
    // REG_51 <9>   0 Auto CDCSS Bw Mode              1 = Disable                        0 = Enable.
    // REG_51 <8>   0 Auto CTCSS Bw Mode              0 = Enable                         1 = Disable
    // REG_51 <6:0> 0 CTCSS/CDCSS Tx Gain1 Tuning     0 = min                            127 = max
    BK4819_WriteRegister(BK4819_REG_51, 0
        | BK4819_REG_51_DISABLE_CxCSS
        | BK4819_REG_51_GPIO6_PIN2_NORMAL
        | BK4819_REG_51_TX_CDCSS_POSITIVE
        | BK4819_REG_51_MODE_CDCSS
        | BK4819_REG_51_CDCSS_23_BIT
        | BK4819_REG_51_1050HZ_NO_DETECTION
        | BK4819_REG_51_AUTO_CDCSS_BW_DISABLE
        | BK4819_REG_51_AUTO_CTCSS_BW_DISABLE);
    BK4819_RX_TurnOn();
}
// -----------------------------------------------------------------------------
void BK4819_Disable(void){
    BK4819_WriteRegister(BK4819_REG_30, 0);
}
// -----------------------------------------------------------------------------
void BK4819_StopScan(void){
    BK4819_DisableFrequencyScan();
    BK4819_Disable();
}
// -----------------------------------------------------------------------------
uint8_t BK4819_GetDTMF_5TONE_Code(void){
    return (BK4819_ReadRegister(BK4819_REG_0B) >> 8) & 0x0F;
}
// -----------------------------------------------------------------------------
uint8_t BK4819_GetCDCSSCodeType(void){
    return (BK4819_ReadRegister(BK4819_REG_0C) >> 14) & 3u;
}
// -----------------------------------------------------------------------------
uint8_t BK4819_GetCTCShift(void){
    return (BK4819_ReadRegister(BK4819_REG_0C) >> 12) & 3u;
}
// -----------------------------------------------------------------------------
uint8_t BK4819_GetCTCType(void){
    return (BK4819_ReadRegister(BK4819_REG_0C) >> 10) & 3u;
}
// -----------------------------------------------------------------------------
void BK4819_SendFSKData(uint16_t *pData){
    unsigned int i;
    uint8_t Timeout = 200;
    esp_rom_delay_us(20000);
    BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_FSK_TX_FINISHED);
    BK4819_WriteRegister(BK4819_REG_59, 0x8068);
    BK4819_WriteRegister(BK4819_REG_59, 0x0068);
    for (i = 0; i < 36; i++){
        BK4819_WriteRegister(BK4819_REG_5F, pData[i]);
    }
    esp_rom_delay_us(20000);
    BK4819_WriteRegister(BK4819_REG_59, 0x2868);
    while (Timeout-- && (BK4819_ReadRegister(BK4819_REG_0C) & 1u) == 0){
        esp_rom_delay_us(5000);
    }
    BK4819_WriteRegister(BK4819_REG_02, 0);
    esp_rom_delay_us(20000);
    BK4819_ResetFSK();
}
// -----------------------------------------------------------------------------
void BK4819_PrepareFSKReceive(void){
    BK4819_ResetFSK();
    BK4819_WriteRegister(BK4819_REG_02, 0);
    BK4819_WriteRegister(BK4819_REG_3F, 0);
    BK4819_RX_TurnOn();
    BK4819_WriteRegister(BK4819_REG_3F, 0 | BK4819_REG_3F_FSK_RX_FINISHED | BK4819_REG_3F_FSK_FIFO_ALMOST_FULL);
    // Clear RX FIFO
    // FSK Preamble Length 7 bytes
    // FSK SyncLength Selection
    BK4819_WriteRegister(BK4819_REG_59, 0x4068);
    // Enable FSK Scramble
    // Enable FSK RX
    // FSK Preamble Length 7 bytes
    // FSK SyncLength Selection
    BK4819_WriteRegister(BK4819_REG_59, 0x3068);
}
// -----------------------------------------------------------------------------
void BK4819_PlayRoger(void){
    #if 0
        const uint32_t tone1_Hz = 500;
        const uint32_t tone2_Hz = 700;
    #else
        // motorola type
        const uint32_t tone1_Hz = 1540;
        const uint32_t tone2_Hz = 1310;
    #endif
    BK4819_EnterTxMute();
    BK4819_SetAF(BK4819_AF_MUTE);
    BK4819_WriteRegister(BK4819_REG_70, 0xE000);  // 1110 0000 0000 0000
    BK4819_EnableTXLink();
    esp_rom_delay_us(50000);
    BK4819_WriteRegister(BK4819_REG_71, scale_freq(tone1_Hz));
    BK4819_ExitTxMute();
    esp_rom_delay_us(80000);
    BK4819_EnterTxMute();
    BK4819_WriteRegister(BK4819_REG_71, scale_freq(tone2_Hz));
    BK4819_ExitTxMute();
    esp_rom_delay_us(80000);
    BK4819_EnterTxMute();
    BK4819_WriteRegister(BK4819_REG_70, 0x0000);
    BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);   // 1 1 0000 0 1 1111 1 1 1 0
}
// -----------------------------------------------------------------------------
// 
//      MDC
//     
// -----------------------------------------------------------------------------


unsigned char  MDC_SYNC[5];
unsigned int MDC_TXDATA[MDC_LEN];
unsigned int MDC_RXDATA[MDC_LEN];
// -----------------------------------------------------------------------------
//void BK4819_FskIdle(){
    //BK4819_WriteRegister(BK4819_REG_3F,0x0000); //tx sucs irq mask=0
    //BK4819_WriteRegister(BK4819_REG_59,REG_59); //fsk_tx_en=0, fsk_rx_en=0
    //BK4819_Idle();
//}
// -----------------------------------------------------------------------------
void BK4819_EnterMdc(){
#if MDC2400 //1200/2400
    BK4819_WriteRegister(0x58,0x73C1); 
    BK4819_WriteRegister(0x72,FSKBUAD<<1);
#else //1200/1800
    BK4819_WriteRegister(BK4819_REG_58,0x37C3); 
    BK4819_WriteRegister(BK4819_REG_72,FSKBUAD);
#endif //MDC2400
    BK4819_WriteRegister(BK4819_REG_70,0x00E0); //[7]=1,Enable Tone2 for FFSK; [6:0]=Gain
    BK4819_WriteRegister(BK4819_REG_5D,((MDC_LEN*2-1)<<8)); //[15:8]ffsk tx length(byte)
    MDC_SYNC[0] = 0x0020;
    MDC_SYNC[1] = 0x0021;
    MDC_SYNC[2] = 0x0022;
    MDC_SYNC[3] = 0x0023;
    MDC_SYNC[4] = 0x0024;
}
// -----------------------------------------------------------------------------
void BK4819_ExitMdc(){
    BK4819_WriteRegister(BK4819_REG_58,0x0000); //Disable FFSK
    BK4819_WriteRegister(BK4819_REG_70,0x0000); //Disable Tone2 
}
// -----------------------------------------------------------------------------
unsigned char BK4819_MdcTransmit(){
    //RF_Txon();
    BK4819_WriteRegister(BK4819_REG_3F,0x8000); //tx sucs irq mask=1
    BK4819_WriteRegister(BK4819_REG_59,REG_59 | 0x8000); //[15]fifo clear; [7:4]prmb_size
    BK4819_WriteRegister(BK4819_REG_59,REG_59);
    unsigned int rdata = 0;
    unsigned char  cnt = 200; //~=1s protection
    //Set Sync 40 bits
    MDC_SYNC[0] = 0x0020;
    MDC_SYNC[1] = 0x0021;
    MDC_SYNC[2] = 0x0022;
    MDC_SYNC[3] = 0x0023;
    MDC_SYNC[4] = 0x0024;
    rdata  = MDC_SYNC[0]; 
    rdata <<= 8; 
    rdata |= MDC_SYNC[1];
    BK4819_WriteRegister(BK4819_REG_5A,rdata);
    
    rdata  = MDC_SYNC[2]; 
    rdata <<= 8; 
    rdata |= MDC_SYNC[3];
    BK4819_WriteRegister(BK4819_REG_5B,rdata);
    
    rdata  = MDC_SYNC[4]; 
    rdata <<= 8; 
    rdata |= 0x30       ;  
    BK4819_WriteRegister(BK4819_REG_5C,rdata);
    unsigned char i;
    MDC_TXDATA[0] =  0x0102;
    MDC_TXDATA[1] =  0x0304;
    MDC_TXDATA[2] =  0x0506;
    MDC_TXDATA[3] =  0x0708;
    MDC_TXDATA[4] =  0x090A;
    MDC_TXDATA[5] =  0x0B0C;
    MDC_TXDATA[6] =  0x0D0E;
    MDC_TXDATA[7] =  0x0F00;
    for(i=0;i<MDC_LEN;i++){
        BK4819_WriteRegister(BK4819_REG_5F,MDC_TXDATA[i]); //push data to fifo
    }
    esp_rom_delay_us(20000);
    BK4819_WriteRegister(BK4819_REG_59,REG_59 | 0x0800); //[11]fsk_tx_en;[13]scrb=0
    while(cnt && !(rdata&0x1)){
        esp_rom_delay_us(5000);
        rdata =BK4819_ReadRegister(BK4819_REG_0C);
    cnt--;
    }
    BK4819_WriteRegister(BK4819_REG_02,0x0000); //clear int
    BK4819_FskIdle();
    if(!cnt){
        //$display("FSK Tx FAILED.."); 
        return 1;
    }
    else {
        //$display("FSK Tx SUCCEED..");
        return 0;
    }
}
// -----------------------------------------------------------------------------
unsigned char BK4819_MdcReceive(){
    //RF_Rxon();
    BK4819_WriteRegister(BK4819_REG_59,REG_59 | 0x4000); //[14]fifo clear
    BK4819_WriteRegister(BK4819_REG_59,REG_59 | 0x1000); //[12]fsk_rx_en;[13]scrb=0
    BK4819_WriteRegister(BK4819_REG_3F,0x3000); //rx sucs/fifo_af irq mask=1
    memset(MDC_RXDATA,0,sizeof(MDC_RXDATA));
    unsigned int rdata;
    unsigned int cnt; 
    unsigned char  i,j,k=0;
    for(i=0;i<(MDC_LEN>>2);i++){
        rdata = 0;
        cnt = 200; //~=1s protection
        while(cnt && !(rdata&0x1)){
            esp_rom_delay_us(5000);
            rdata = BK4819_ReadRegister(BK4819_REG_02);
            cnt--;
        }
        BK4819_WriteRegister(BK4819_REG_02,0x0000); //clear int
        if(!cnt){
            BK4819_FskIdle(); 
            return 1;
        }
        for(j=0;j<4;j++){
           rdata = BK4819_ReadRegister(BK4819_REG_5F); //pop data from fifo
           MDC_RXDATA[k] = rdata;
           k++;
        }
    }
    rdata = 0;
    cnt = 200; //~=1s protection
    while(cnt && !(rdata&0x1)){
        esp_rom_delay_us(5000);
        rdata = BK4819_ReadRegister(BK4819_REG_02);
        cnt--;
    }
    BK4819_WriteRegister(0x02,0x0000); //clear int
    if(!cnt){
        BK4819_FskIdle(); 
        return 1;
    }
    cnt = MDC_LEN & 3;
    while(cnt){
        rdata = BK4819_ReadRegister(BK4819_REG_5F); //pop data from fifo
        MDC_RXDATA[k] = rdata;
        k++;
        cnt--;
    }
    //$display("FFSK Rx SUCCEED..");
    rdata = BK4819_ReadRegister(0x8D); //[15:8]Sync[4]
    BK4819_FskIdle();
    if((rdata>>8)^MDC_SYNC[4]){
        return 1;//$display("FSK Sync Fail.");
    }
    return 0;
}
// -----------------------------------------------------------------------------
// 
//      FSK
//     
// -----------------------------------------------------------------------------
#define FSKBUAD   0x3065 //1200*10.32444; 10.32444 for 13M/26M and 10.48576 for 12.8M/19.2M/25.6M/38.4M
#define FSK_LEN   5      //0~127, numbers_of_byte=(FSK_LEN*2)
#define FSK2400   0      //0=1200;1=2400

unsigned int FSK_TXDATA[FSK_LEN];
unsigned int FSK_RXDATA[127];
// ----------------------------------------------------------------------------
/*void RF_EnterFsk(){
    BK4819_WriteRegister(0x70,0x00E0); //[7]=1,Enable Tone2 for FSK; [6:0]=Gain
#if FSK2400
    BK4819_WriteRegister(0x72,FSKBUAD<<1); //2400bps
    BK4819_WriteRegister(0x58,0x00C9);
#else
    BK4819_WriteRegister(0x72,FSKBUAD);    //1200bps
    BK4819_WriteRegister(0x58,0x00C1);
#endif //FSK2400
    //BK4819_WriteRegister(0x5C,0x5665); // 1010110 01100101
    BK4819_WriteRegister(0x5D,(FSK_LEN*2-1)<<8); //[15:8]fsk tx length(byte)
}*/
    //BK4819_WriteRegister(0x5C,0x5665); // 1010110 01100101
/*void RF_ExitFsk(){
    BK4819_WriteRegister(0x70,0x0000); //Disable Tone2 
    BK4819_WriteRegister(0x58,0x0000); //Disable FSK
}
// ----------------------------------------------------------------------------
void RF_MyFskIdle(){
    BK4819_WriteRegister(0x3F,0x0000); //tx sucs irq mask=0
    BK4819_WriteRegister(0x59,REG_59); //fsk_tx_en=0, fsk_rx_en=0
    BK4819_WriteRegister(0x30,0x0000); //RF Idle(current~=3mA)
}*/
// ----------------------------------------------------------------------------
unsigned char RF_MyFskTransmit(){
    unsigned int rdata;
    // TX ON -------------
    //BK4819_WriteRegister(0x37,REG_37 | 0xF); //[1]xtal;[0]bg;[9]ldo_rf_vsel=0 when txon
    BK4819_WriteRegister(0x52,REG_52);       //Set bit[15]=0 to Clear ctcss/cdcss Tail
    BK4819_WriteRegister(0x30,0x0000);
    BK4819_WriteRegister(0x30,0xC1FE);  
    // -------------------
    BK4819_WriteRegister(0x3F,0x8000); //tx sucs irq mask=1
    BK4819_WriteRegister(0x59,REG_59 | 0x8002); //[15]fifo clear
    BK4819_WriteRegister(0x59,REG_59 | 0x02);
    
    BK4819_WriteRegister(BK4819_REG_5A, 0x5555);   // First two sync bytes
    BK4819_WriteRegister(BK4819_REG_5B, 0x55AA);   // End of sync bytes. Total 4 bytes: 555555aa
    //BK4819_WriteRegister(BK4819_REG_5C, 0xAA40);   // Enable CRC
    BK4819_WriteRegister(BK4819_REG_5C, 0xAA65);
    // -----------------------------------------
    unsigned char i;
    FSK_TXDATA[0] = 0x1122;
    FSK_TXDATA[1] = 0x3344;
    FSK_TXDATA[2] = 0x5566;
    FSK_TXDATA[3] = 0x7788;
    FSK_TXDATA[4] = 0x99AA;
    for(i=0;i<FSK_LEN;i++){
        BK4819_WriteRegister(0x5F,FSK_TXDATA[i]); //push data to fifo
    }
    esp_rom_delay_us(20000);
    BK4819_WriteRegister(0x59,REG_59 | 0x802); //[11]fsk_tx_en;

    unsigned char  cnt = 200; //~=1s protection
    rdata = 0;
    while(cnt && !(rdata&0x1)){
        esp_rom_delay_us(5000);
        rdata = BK4819_ReadRegister(0x0C);
        cnt--;
    }
    BK4819_WriteRegister(0x02,0x0000); //clear int
    RF_MyFskIdle();
    if(!cnt){
        //$display("FSK Tx FAILED..");
        return 1;
    }
    else{
        //$display("FSK Tx SUCCEED..");
        return 0;
    }
}
// ----------------------------------------------------------------------------
unsigned char RF_MyFskReceive(){
    // RX ON ------------------------
    //BK4819_WriteRegister(0x37,REG_37 | 0xF | 1<<9); //[1]xtal;[0]bg;[9]ldo_rf_vsel=1 when rxon
    BK4819_WriteRegister(0x30,0x0000);
    BK4819_WriteRegister(0x30,0xBFF1);  //RF Rxon
    // ------------------------------
    BK4819_WriteRegister(0x59,REG_59 | 0x4002); //[14]fifo clear
    BK4819_WriteRegister(0x59,REG_59 | 0x1002); //[12]fsk_rx_en;[13]scrb=1
    BK4819_WriteRegister(0x3F,0x3000); //rx sucs/fifo_af irq mask=1
    
    BK4819_WriteRegister(BK4819_REG_5A, 0x5555);   // First two sync bytes
    BK4819_WriteRegister(BK4819_REG_5B, 0x55AA);   // End of sync bytes. Total 4 bytes: 555555aa
    // -----------------------------------------
    //BK4819_WriteRegister(BK4819_REG_5C, 0xAA40);   // Enable CRC
    BK4819_WriteRegister(BK4819_REG_5C, 0xAA65);
    // -----------------------------------------
    memset(&FSK_RXDATA,0,sizeof(FSK_RXDATA));
    unsigned int cnt, rdata; 
    unsigned char  i,j,k=0;
    for(i=0;i<(FSK_LEN>>2);i++){
        rdata = 0;
        cnt = 2000; //~=1s protection
        while(cnt && !(rdata&0x1)){
            esp_rom_delay_us(1000);
            rdata = BK4819_ReadRegister(0x0C);
            cnt--;
        }
        BK4819_WriteRegister(0x02,0x0000); //clear int
        if(!cnt){
            //RF_MyFskIdle(); 
            return 1;
        }
        for(j=0;j<(FSK_LEN-1);j++){
           rdata = BK4819_ReadRegister(0x5F); //pop data from fifo
           FSK_RXDATA[k] = rdata;
           k++;
        }
    }
    rdata = 0;
    cnt = 200; //~=1s protection
    while(cnt && !(rdata&0x1)){
        esp_rom_delay_us(5000);
        rdata = BK4819_ReadRegister(0x0C);
        cnt--;
    }
    BK4819_WriteRegister(0x02,0x0000);
    if(!cnt){
        //RF_MyFskIdle(); 
        return 1;
    }
    cnt = FSK_LEN & 3;
    while(cnt){
        rdata = BK4819_ReadRegister(0x5F); //pop data from fifo
        FSK_RXDATA[k] = rdata;
        k++;
        cnt--;
    }
    //$display("FSK Rx SUCCEED..");
    rdata = BK4819_ReadRegister(0x0B); //[4]crc
    //RF_MyFskIdle();
    if(!(rdata&0x10)){
        return 1;//$display("FSK CRC16 Check Fail.");
    }
    return 0;
}
// -----------------------------------------------------------------------------
//
//		DTMF
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
void BK4819_Initial(){
    //Soft Reset RF
    BK4819_WriteRegister(0x00,0x8000);
    BK4819_WriteRegister(0x00,0x0000);
    //Power Up RF
    BK4819_WriteRegister(0x37,REG_37 | 0xF); 
    //Set SQ Threshold if necessary
    //BK4819_WriteRegister(0x4D,REG_4D | TH_GLITCH_SQ_IN ); //??Glitch?????? 
    //BK4819_WriteRegister(0x4E,REG_4E | TH_GLITCH_SQ_OUT); //??Glitch??????
    //BK4819_WriteRegister(0x4F,TH_NOISE_SQ_OUT<<8 | TH_NOISE_SQ_IN ); //??Noise????????
    //BK4819_WriteRegister(0x78,TH_RSSI_SQ_IN<<8   | TH_RSSI_SQ_OUT ); //??Rssi????????,????????
    //Set Deviation if necessary
    //BK4819_WriteRegister(0x40,REG_40 | DEVIATION);  //bit[12]=Dev_en????;bit[11:0]=Deviation_lvl????
    //Set Tx Power if necessary
    //BK4819_WriteRegister(0x36,PACTL<<8 | PATCL_EN<<7 | PA_GAIN); //bit[15:8]=PACTL????;bit[7]=PACTLEn????;[5:0]=PaGain????
    //Set AGC Table
    BK4819_WriteRegister(0x13,0x03BE);
    BK4819_WriteRegister(0x12,0x037B);
    BK4819_WriteRegister(0x11,0x027B);
    BK4819_WriteRegister(0x10,0x007A);
    BK4819_WriteRegister(0x14,0x0019);
    BK4819_WriteRegister(0x49,0x2A38);
    BK4819_WriteRegister(0x7B,0x8420);
    //Set CTCSS Threshold if necessary
    //BK4819_WriteRegister(0x52,REG_52); //CTCSS????????
    //Set MIC Sensitivity
    BK4819_WriteRegister(0x7D,MIC_GAIN | REG_7D); //bit[4:0]=MicSens???
    //Set Volume 
    BK4819_WriteRegister(0x48,REG_48 | VOL_GAIN<<4 | DAC_GAIN); //bit[9:4]=??????;bit[3:0]??????
    //Set XTAL Frequency
    //RF_SetXtal(XTAL26M);
    //Others Setting
    BK4819_WriteRegister(0x1F,0x5454); 	//2020.9.5 set rfpll_regvco_vbit=0101 BK4819_WriteRegister(0x1F,0x1454);
    //2020.8.24 BK4819_WriteRegister(0x1F,0x1858);  
    BK4819_WriteRegister(0x25,0xC1BA);
    BK4819_WriteRegister(0x77,0x88EF);
    BK4819_WriteRegister(0x3E,0x9896); //0x9896 for smic 0xa037 for gf
}
// -----------------------------------------------------------------------------
void BK4819_EnterDtmf(){
    //Set DTMF Symbol Rx Coefficent
    //bit7:0=cos(3*PI*f*FACTOR/1024/128)*64*2; FACTOR=10.32444 for 13M/26M and FACTOR=10.48576 for 12.8M/19.2M/25.6M/38.4M
    BK4819_WriteRegister(0x09,0x6F | 0x0<<12); 
    BK4819_WriteRegister(0x09,0x6B | 0x1<<12);
    BK4819_WriteRegister(0x09,0x67 | 0x2<<12);
    BK4819_WriteRegister(0x09,0x62 | 0x3<<12);
    BK4819_WriteRegister(0x09,0x50 | 0x4<<12);
    BK4819_WriteRegister(0x09,0x47 | 0x5<<12);
    BK4819_WriteRegister(0x09,0x3A | 0x6<<12);
    BK4819_WriteRegister(0x09,0x2C | 0x7<<12);
    BK4819_WriteRegister(0x09,0x41 | 0x8<<12);
    BK4819_WriteRegister(0x09,0x37 | 0x9<<12);
    BK4819_WriteRegister(0x09,0x25 | 0xA<<12);
    BK4819_WriteRegister(0x09,0x17 | 0xB<<12);
    BK4819_WriteRegister(0x09,0xE4 | 0xC<<12);
    BK4819_WriteRegister(0x09,0xCB | 0xD<<12);
    BK4819_WriteRegister(0x09,0xB5 | 0xE<<12);
    BK4819_WriteRegister(0x09,0x9F | 0xF<<12);
    BK4819_WriteRegister(0x21,0x06D8); 
    BK4819_WriteRegister(0x24,0x807E | DTMF_TH<<7); //[12:7]threshold
}
// -----------------------------------------------------------------------------
void BK4819_ExitDtmf(){
    BK4819_WriteRegister(0x24,0x0000); //Disable DTMF
}
// -----------------------------------------------------------------------------
void BK4819_DtmfTransmit(){
    BK4819_WriteRegister(0x47,REG_47 | (unsigned int) (3<<8));
    BK4819_WriteRegister(0x70,0xE0E0); //Enable Tone1 & Tone2
    BK4819_WriteRegister(0x50,REG_50 | 1<<15);
    BK4819_WriteRegister(0x37,REG_37 | 0xF); //[1]xtal;[0]bg;[9]ldo_rf_vsel=0 when txon
    BK4819_WriteRegister(0x52,REG_52);  //Set bit[15]=0 to Clear ctcss/cdcss Tail
    BK4819_WriteRegister(0x30,0x0000);
    BK4819_WriteRegister(0x30,0xC1FA | 1<<9);  //RF Txon; Set bit[9]=1(AF) bit[2]=0(MIC) for BEEP mode
    for(unsigned char i=0;i<=DTMF_LEN;i++){
        esp_rom_delay_us(50000);
        switch(DTMF_SYMBOL[i]){
            case 0:
                    BK4819_WriteRegister(0x71,FREQ_941 ); //FREQ_941=941*10.32444; 10.32444 for 13M/26M and 10.48576 for 12.8M/19.2M/25.6M/38.4M
                    BK4819_WriteRegister(0x72,FREQ_1336);
                    break;
            case 1:
                    BK4819_WriteRegister(0x71,FREQ_697 ); 
                    BK4819_WriteRegister(0x72,FREQ_1209);
                    break;
            case 2:
                    BK4819_WriteRegister(0x71,FREQ_697 ); 
                    BK4819_WriteRegister(0x72,FREQ_1336);
                    break;
            case 3:
                    BK4819_WriteRegister(0x71,FREQ_697 ); 
                    BK4819_WriteRegister(0x72,FREQ_1477);
                    break;
            case 4:
                    BK4819_WriteRegister(0x71,FREQ_770 ); 
                    BK4819_WriteRegister(0x72,FREQ_1209);
                    break;
            case 5:
                    BK4819_WriteRegister(0x71,FREQ_770 ); 
                    BK4819_WriteRegister(0x72,FREQ_1336);
                    break;
            case 6:
                    BK4819_WriteRegister(0x71,FREQ_770 ); 
                    BK4819_WriteRegister(0x72,FREQ_1477);
                    break;
            case 7:
                    BK4819_WriteRegister(0x71,FREQ_852 ); 
                    BK4819_WriteRegister(0x72,FREQ_1209);
                    break;
            case 8:
                    BK4819_WriteRegister(0x71,FREQ_852 ); 
                    BK4819_WriteRegister(0x72,FREQ_1336);
                    break;
            case 9:
                    BK4819_WriteRegister(0x71,FREQ_852 ); 
                    BK4819_WriteRegister(0x72,FREQ_1477);
                    break;
            case 0xA:
                    BK4819_WriteRegister(0x71,FREQ_697 ); 
                    BK4819_WriteRegister(0x72,FREQ_1633);
                    break;
            case 0xB:
                    BK4819_WriteRegister(0x71,FREQ_770 ); 
                    BK4819_WriteRegister(0x72,FREQ_1633);
                    break;
            case 0xC:
                    BK4819_WriteRegister(0x71,FREQ_852 ); 
                    BK4819_WriteRegister(0x72,FREQ_1633);
                    break;
            case 0xD:
                    BK4819_WriteRegister(0x71,FREQ_941 ); 
                    BK4819_WriteRegister(0x72,FREQ_1633);
                    break;
            case 0xE: //'*'
                    BK4819_WriteRegister(0x71,FREQ_941 ); 
                    BK4819_WriteRegister(0x72,FREQ_1209);
                    break;
            case 0xF: //'#'
                    BK4819_WriteRegister(0x71,FREQ_941 ); 
                    BK4819_WriteRegister(0x72,FREQ_1477);
                    break;
        }
        BK4819_WriteRegister(0x50,REG_50);
        esp_rom_delay_us(50000);
        BK4819_WriteRegister(0x50,REG_50 | 1<<15);
    }
    esp_rom_delay_us(50000);
    BK4819_WriteRegister(0x30,0x0000); //RF Idle(current~=3mA)
    BK4819_WriteRegister(0x47,REG_47 | (unsigned int) (1<<8));
    BK4819_WriteRegister(0x50,REG_50);
    BK4819_WriteRegister(0x70,0x0000); //Disable Tone1 & Tone2
}
// -----------------------------------------------------------------------------
unsigned char BK4819_DtmfReceive(){
    BK4819_WriteRegister(0x37,REG_37 | 0xF | 1<<9); //[1]xtal;[0]bg;[9]ldo_rf_vsel=1 when rxon
    //RF_SetAf(MUTE);
    BK4819_WriteRegister(0x30,0x0000);
    BK4819_WriteRegister(0x30,0xBFF1);  //RF Rxon
    //when SQ and CTC/DCS is link
    //OpenAudioPA here...
    //__delay_ms(75);
    //RF_SetAf(OPEN);
    //when SQ or CTC/DCS is lost or CTC/DCS Tail is found
    //ShutAudioPA here...
    BK4819_WriteRegister(0x3F,0x0800); //dtmf interrupt mask
    unsigned int rdata;
    unsigned char  cnt; 
    unsigned char  i;
    for(i=0;i<DTMF_LEN;i++){
        rdata = 0;
        cnt = 1000;  //5s protection
        while(cnt && !(rdata&0x1)){
            esp_rom_delay_us(5000);
            rdata = BK4819_ReadRegister(0x0C);
            cnt--;		
        }
        if(!cnt){
            BK4819_WriteRegister(0x3F,0x0000); //dtmf interrupt mask 
            BK4819_WriteRegister(0x30,0x0000); //RF Idle(current~=3mA)
            return 1;
        }
        BK4819_WriteRegister(0x02,0x0000); //clear int
        rdata = BK4819_ReadRegister(0x0B);
        DTMF_RXDATA[i] = (rdata>>8)&0xF; //[11:8]dtmf_code
    }
    BK4819_WriteRegister(0x3F,0x0000); //dtmf interrupt mask 
    BK4819_WriteRegister(0x30,0x0000); //RF Idle(current~=3mA)
    return 0;
}
// -----------------------------------------------------------------------------
void BK4819_PlayRogerMDC(void){
    unsigned int i;
    BK4819_SetAF(BK4819_AF_MUTE);
    BK4819_WriteRegister(BK4819_REG_58, 0x37C3);   // FSK Enable,
                                                   // RX Bandwidth FFSK 1200/1800
                                                   // 0xAA or 0x55 Preamble
                                                   // 11 RX Gain,
                                                   // 101 RX Mode
                                                   // TX FFSK 1200/1800
    BK4819_WriteRegister(BK4819_REG_72, 0x3065);   // Set Tone-2 to 1200Hz
    BK4819_WriteRegister(BK4819_REG_70, 0x00E0);   // Enable Tone-2 and Set Tone2 Gain
    BK4819_WriteRegister(BK4819_REG_5D, 0x0D00);   // Set FSK data length to 13 bytes
    BK4819_WriteRegister(BK4819_REG_59, 0x8068);   // 4 byte sync length, 6 byte preamble, clear TX FIFO
    BK4819_WriteRegister(BK4819_REG_59, 0x0068);   // Same, but clear TX FIFO is now unset (clearing done)
    BK4819_WriteRegister(BK4819_REG_5A, 0x5555);   // First two sync bytes
    BK4819_WriteRegister(BK4819_REG_5B, 0x55AA);   // End of sync bytes. Total 4 bytes: 555555aa
    BK4819_WriteRegister(BK4819_REG_5C, 0xAA30);   // Disable CRC
    // Send the data from the roger table
    for (i = 0; i < 7; i++){
        BK4819_WriteRegister(BK4819_REG_5F, FSK_RogerTable[i]);
    }
    esp_rom_delay_us(20000);
    // 4 sync bytes, 6 byte preamble, Enable FSK TX
    BK4819_WriteRegister(BK4819_REG_59, 0x0868);
    esp_rom_delay_us(180000);
    // Stop FSK TX, reset Tone-2, disable FSK
    BK4819_WriteRegister(BK4819_REG_59, 0x0068);
    BK4819_WriteRegister(BK4819_REG_70, 0x0000);
    BK4819_WriteRegister(BK4819_REG_58, 0x0000);
}
// -----------------------------------------------------------------------------
void BK4819_Enable_AfDac_DiscMode_TxDsp(void){
    BK4819_WriteRegister(BK4819_REG_30, 0x0000);
    BK4819_WriteRegister(BK4819_REG_30, 0x0302);
}
// -----------------------------------------------------------------------------
void BK4819_GetVoxAmp(uint16_t *pResult){
    *pResult = BK4819_ReadRegister(BK4819_REG_64) & 0x7FFF;
}
// -----------------------------------------------------------------------------
void BK4819_SetScrambleFrequencyControlWord(uint32_t Frequency){
    BK4819_WriteRegister(BK4819_REG_71, scale_freq(Frequency));
}
// -----------------------------------------------------------------------------
void BK4819_PlayDTMFEx(bool bLocalLoopback, char Code){
    BK4819_EnableDTMF();
    BK4819_EnterTxMute();
    BK4819_SetAF(bLocalLoopback ? BK4819_AF_BEEP : BK4819_AF_MUTE);
    BK4819_WriteRegister(BK4819_REG_70, 0xD3D3);
    BK4819_EnableTXLink();
    esp_rom_delay_us(50000);
    BK4819_PlayDTMF(Code);
    BK4819_ExitTxMute();
}
// -----------------------------------------------------------------------------
/** The BK4819 can send FSK packets with (pag.10 of 'BK4819(V3) Application Note 20210428.pdf'):
 * 1 to 16 preamble bytes
 * 2 or 4 sync bytes
 * 1 to 1024 words of payload (1 word = 2 bytes)
 * optional 2 bytes CRC
 * [1-16 B preamble][2|4 B sync][1-1024 W payload][0|2 B CRC]
 * 
 ** The BK4819 has the following FIFO buffers (pag.11 of 'BK4819(V3) Application Note 20210428.pdf'):
 * TX: 128 words
 * RX:   8 words
 * 
 ** The FIFO TX and RX buffer thresholds that fire the interrupts are configurable:
 * TX: REG_5E<9:3> // BK4819_REG_5E_MASK_FSK_TX_FIFO_THRESHOLD // default 64 words
 * RX: REG_5E<2:0> // BK4819_REG_5E_MASK_FSK_RX_FIFO_THRESHOLD // default  4 words
 **/
/*
void BK4819_FskEnterMode(
    FSK_TX_RX_t txRx,
    FSK_MODULATION_TYPE_t fskModulationType,
    uint8_t fskTone2Gain,       // 0-127
    FSK_NO_SYNC_BYTES_t fskNoSyncBytes, // 0 (2 bytes) or 1 (4 bytes)
    uint8_t fskNoPreambleBytes, // 1-16 bytes
    bool fskScrambleEnable,
    bool fskCrcEnable,
    bool fskInvertData
    ){
    // Enable Tone2 and Set Tone2 Gain
    BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE2 | fskTone2Gain); // Tone2 gain: 0-127
    if(fskModulationType == FSK_MODULATION_TYPE_FSK1K2 || fskModulationType == FSK_MODULATION_TYPE_MSK1200_1800){
        BK4819_WriteRegister(BK4819_REG_72, scale_freq(1200)); // FSK 1K2 and MSK 1200/1800 are at 1200 bps
    }
    else{
        //void BK4819_WriteRegister(BK4819_REGISTER_t Register, uint16_t Data)
        BK4819_WriteRegister((BK4819_REGISTER_t)BK4819_REG_72, scale_freq((uint16_t)2400)); // FSK 2K4 and MSK 1200/2400 are at 2400 bps
    }
    // FSK Enable, RX Bandwidth FFSK1200/1800, 0xAA or 0x55 Preamble, 11 RX Gain,
    // 101 RX Mode, FFSK1200/1800 TX
    //| BK4819_REG_58_FSK_RX_GAIN_2 
    #define REG58_COMMON_SETTINGS ( \
        BK4819_REG_58_FSK_PREAMBLE_TYPE_0xAA_OR_0x55 \
      | BK4819_REG_58_FSK_ENABLE \
      | BK4819_REG_58_FSK_RX_GAIN_2 \
      | BK4819_REG_58_MASK_FSK_UNKNOWN ) // in LVG github code we find '0b11'
    switch(fskModulationType) {
        case FSK_MODULATION_TYPE_FSK1K2 : // AFSK 1200 bps
            if(txRx == FSK_RX){
                BK4819_WriteRegister(BK4819_REG_58, REG58_COMMON_SETTINGS
                | BK4819_REG_58_FSK_RX_MODE_FSK1200_FSK2400_NOAA
                | BK4819_REG_58_FSK_RX_BANDWIDTH_FSK1200 );
            }
            else{
                BK4819_WriteRegister(BK4819_REG_58, REG58_COMMON_SETTINGS | BK4819_REG_58_FSK_TX_MODE_FSK1200_FSK2400 );
            }
            break;
        case FSK_MODULATION_TYPE_FSK2K4 : // AFSK 2400 bps
            if(txRx == FSK_RX){
                BK4819_WriteRegister(BK4819_REG_58, REG58_COMMON_SETTINGS
                        | BK4819_REG_58_FSK_RX_MODE_FSK1200_FSK2400_NOAA
                        | BK4819_REG_58_FSK_RX_BANDWIDTH_FSK2400_OR_FFSK1200_2400 );
            }
            else{
                BK4819_WriteRegister(BK4819_REG_58, REG58_COMMON_SETTINGS | BK4819_REG_58_FSK_TX_MODE_FSK1200_FSK2400 );
            }
            break;
        case FSK_MODULATION_TYPE_MSK1200_1800 : // MSK 1200 bps
            if(txRx == FSK_RX){
                BK4819_WriteRegister(BK4819_REG_58, REG58_COMMON_SETTINGS 
                        | BK4819_REG_58_FSK_RX_MODE_FFSK1200_1800
                        | BK4819_REG_58_FSK_RX_BANDWIDTH_FFSK1200_1800 );
            }
            else{
                BK4819_WriteRegister(BK4819_REG_58, REG58_COMMON_SETTINGS | BK4819_REG_58_FSK_TX_MODE_FFSK1200_1800);
            }
            break;
        case FSK_MODULATION_TYPE_MSK1200_2400 : // MSK 2400 bps
            if(txRx == FSK_RX){
                BK4819_WriteRegister(BK4819_REG_58, REG58_COMMON_SETTINGS 
                        | BK4819_REG_58_FSK_RX_MODE_FFSK1200_2400
                        | BK4819_REG_58_FSK_RX_BANDWIDTH_FSK2400_OR_FFSK1200_2400 );
            }
            else{
                BK4819_WriteRegister(BK4819_REG_58, REG58_COMMON_SETTINGS | BK4819_REG_58_FSK_TX_MODE_FFSK1200_2400);
            }
            break;
    }
    // configure the FSK packet (preamble length, sync bytes, scramble)
    //uint16_t reg59_fsk_before = BK4819_ReadRegister(BK4819_REG_59); // TODO: maybe this is not needed
    uint16_t reg59_fsk = (uint16_t) (
           fskScrambleEnable  << BK4819_REG_59_SHIFT_FSK_SCRAMBLE
        | fskNoPreambleBytes << BK4819_REG_59_SHIFT_FSK_PREAMBLE_LENGTH
        | fskNoSyncBytes     << BK4819_REG_59_SHIFT_FSK_SYNC_LENGTH
    );
    if(fskInvertData){
        if(txRx == FSK_RX){
            reg59_fsk |= BK4819_REG_59_MASK_FSK_INVERT_WHEN_RX;
        }
        else{
            reg59_fsk |= BK4819_REG_59_MASK_FSK_INVERT_WHEN_TX;
        }
    }
    BK4819_WriteRegister(BK4819_REG_59, reg59_fsk | BK4819_REG_59_MASK_FSK_CLEAR_TX_FIFO); // TODO: needs to be also written the same register with 0 in clear fifo?
    BK4819_WriteRegister(BK4819_REG_59, reg59_fsk); // in case we need to write the same data in register 59, without the "clear fifo flag"
    // other FSK packet configuration (which sync bytes have to be used - default are 0x85 0xCF 0xAB 0x45)
    // for the moment we use the default sync bytes (0x85 0xCF 0xAB 0x45) 0b1000010111001111 0b1010101101000101
    BK4819_WriteRegister(BK4819_REG_5A, 0x5555);   // First two sync bytes
    BK4819_WriteRegister(BK4819_REG_5B, 0x55AA);   // End of sync bytes. Total 4 bytes: 555555aa
    // setup CRC and other mysterious stuff
    BK4819_WriteRegister(BK4819_REG_5C, 0 | BK4819_REG_5C_MASK_FSK_UNKNOWN | (fskCrcEnable << BK4819_REG_5C_SHIFT_FSK_CRC)); // BK4819_REG_5C_MASK_FSK_OTHER defined in bk4819-regs.h contains values other than CRC that have been found around the code
}
// -----------------------------------------------------------------------------
void BK4819_FskExitMode(void){
    BK4819_WriteRegister(BK4819_REG_70, 0x0000); //Disable Tone2
    BK4819_WriteRegister(BK4819_REG_58, 0x0000); //Disable FSK
}
// -----------------------------------------------------------------------------
void BK4819_FskIdle(void){
    BK4819_WriteRegister(BK4819_REG_3F, (BK4819_ReadRegister(BK4819_REG_3F) & ~(
            BK4819_REG_3F_MASK_FSK_TX_FINISHED 
          | BK4819_REG_3F_MASK_FSK_FIFO_ALMOST_EMPTY
          | BK4819_REG_3F_MASK_FSK_RX_FINISHED
          | BK4819_REG_3F_MASK_FSK_FIFO_ALMOST_FULL
          | BK4819_REG_3F_MASK_FSK_RX_SYNC
    ))); // disable all the FSK-related interrupts
    BK4819_WriteRegister(BK4819_REG_59, (BK4819_ReadRegister(BK4819_REG_59) & ~(
            BK4819_REG_59_MASK_FSK_ENABLE_TX 
          | BK4819_REG_59_MASK_FSK_ENABLE_RX
          | BK4819_REG_59_MASK_FSK_CLEAR_TX_FIFO
    ))); //fsk_tx_en=0, fsk_rx_en=0
}
// -----------------------------------------------------------------------------
FSK_IRQ_t BK4819_FskCheckInterrupt(void){
    uint16_t reg0c_irq = BK4819_ReadRegister(BK4819_REG_0C);
    //printf("BK4819_REG_0C 0x%04x\r\n", reg0c_irq);//0x2803 0010 1000 0000 0011
    if (reg0c_irq & (1u << 0)){
	    // we have some interrupt flags, let's read the REG_02, to check which are
	    uint16_t reg02_irq = BK4819_ReadRegister(BK4819_REG_02);
	    if (reg02_irq != 0x0001){
	        printf("test only\r\n");
	    }
	    if (reg02_irq & BK4819_REG_02_MASK_FSK_TX_FINISHED){
	        printf("interrupt BK4819_REG_02_FSK_TX_FINISHED\r\n");
	        return FSK_TX_FINISHED;
	    }
	    if (reg02_irq & BK4819_REG_02_MASK_FSK_FIFO_ALMOST_EMPTY){
	        printf("interrupt BK4819_REG_02_FSK_FIFO_ALMOST_EMPTY\r\n");
	        return FSK_FIFO_ALMOST_EMPTY;
	    }
	    if (reg02_irq & BK4819_REG_02_MASK_FSK_RX_FINISHED){
	        printf("interrupt BK4819_REG_02_FSK_RX_FINISHED\r\n");
	        return FSK_RX_FINISHED;
	    }
	    if (reg02_irq & BK4819_REG_02_MASK_FSK_FIFO_ALMOST_FULL){
	        printf("interrupt BK4819_REG_02_FSK_FIFO_ALMOST_FULL\r\n");
	        return FSK_FIFO_ALMOST_FULL;
	    }
	    if (reg02_irq & BK4819_REG_02_MASK_FSK_RX_SYNC){
	        printf("interrupt BK4819_REG_02_FSK_RX_SYNC\r\n");
	        return FSK_RX_SYNC;
	    }
	}
    return FSK_OTHER;
}
// -----------------------------------------------------------------------------
int16_t BK4819_FskTransmitPacket(void * tx_buffer_ptr, uint16_t tx_packet_len_bytes){
    #define BK4819_FIFO_DIM_WORDS 	128  // 256 bytes
    #define TX_FIFO_LOW_THRESHOLD_WORDS 64   // 128 bytes --- default is 128 bytes (64 words)
    BK4819_WriteRegister(BK4819_REG_02, 0);
    uint16_t reg5E_fifo = BK4819_ReadRegister(BK4819_REG_5E);
    BK4819_WriteRegister(BK4819_REG_5E, (reg5E_fifo & ~BK4819_REG_5E_MASK_FSK_TX_FIFO_THRESHOLD) | (TX_FIFO_LOW_THRESHOLD_WORDS << BK4819_REG_5E_SHIFT_FSK_TX_FIFO_THRESHOLD));
    BK4819_WriteRegister(BK4819_REG_3F, BK4819_REG_3F_FSK_TX_FINISHED | BK4819_REG_3F_FSK_FIFO_ALMOST_EMPTY); // unfortunately the BK4819_REG_02_FSK_FIFO_ALMOST_FULL is not triggered in TX
    uint16_t reg59_fsk = BK4819_ReadRegister(BK4819_REG_59);
    BK4819_WriteRegister(BK4819_REG_59, reg59_fsk | BK4819_REG_59_MASK_FSK_ENABLE_TX | BK4819_REG_59_MASK_FSK_CLEAR_TX_FIFO);
    int16_t i;
    const uint16_t *p = (const uint16_t *)tx_buffer_ptr; // tx_buffer_ptr can be of whatever type
    for (i = 0; i < (tx_packet_len_bytes / 2); i++){ // i counts the words, not the bytes!
        BK4819_WriteRegister(BK4819_REG_5F, p[i]);  // load 16-bits at a time
    }
    BK4819_WriteRegister(BK4819_REG_59, reg59_fsk | BK4819_REG_59_MASK_FSK_ENABLE_TX);
    uint16_t timeout = 300;
    while (timeout--){
        __delay_ms(1);
        if(BK4819_FskCheckInterrupt() == FSK_TX_FINISHED){
            break;
        }
    }
    __delay_ms(250);
    BK4819_WriteRegister(BK4819_REG_59, reg59_fsk | BK4819_REG_59_MASK_FSK_CLEAR_TX_FIFO);
    BK4819_WriteRegister(BK4819_REG_59, reg59_fsk);
    return i;
}
// -----------------------------------------------------------------------------
unsigned char BK4819_FskReceivePacket(uint16_t * rx_buffer_ptr){
    if (BK4819_FskCheckInterrupt() == FSK_RX_FINISHED){
        BK4819_WriteRegister(BK4819_REG_02, 0);
        unsigned char cnt = MDC_LEN;
        unsigned char rdata;
        while(cnt){
            *rx_buffer_ptr = BK4819_ReadRegister(BK4819_REG_5F); //FIFO
            rx_buffer_ptr++;
            cnt--;
        }
        rdata = BK4819_ReadRegister(0x0B); //CRC
        if(!(rdata&0x10)){
            return 0;
        }
        return 1;
    }
}
*/

#endif