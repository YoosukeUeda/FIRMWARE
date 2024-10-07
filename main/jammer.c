#include "pinout.h"
typedef struct{
    uint16_t* upper;
    uint16_t* lower;
    uint16_t mean;
} JammerRead;
uint8_t jammer_rssi_calibrate[15];

typedef struct {
    uint16_t avg;
    uint16_t stddev;
} JammerPacket;

QueueHandle_t jammer;

JammerRead Jammer_Detect(){ 
    //ESP_LOGI(__func__,"start"); 
    int16_t temp = 0;
    uint16_t mean = 0;
    uint16_t* valueHIGH = malloc(9*sizeof(uint16_t));
    memset(valueHIGH,0,sizeof(&valueHIGH));
    uint16_t* valueLOW = malloc(6*sizeof(uint16_t));
    memset(valueLOW,0,sizeof(&valueLOW));
    uint32_t __freq = 86870000;
    const uint32_t __freq_step = 10000;
    JammerRead jammerRead;

    //Low start range read
    for(int c = 0; c <= 2;c++){
        BK_SetFreq(__freq + c * __freq_step);
        esp_rom_delay_us(50);
        temp = BK4819_GetRSSI();
        valueLOW[c] = temp;
        mean += temp;
        esp_rom_delay_us(10);
    }
    //Low end range read
    __freq = 89410000;
    for(int c = 0; c <= 2;c++){
        BK_SetFreq(__freq + c * __freq_step);
        esp_rom_delay_us(50);
        temp = BK4819_GetRSSI();
        valueLOW[3+c] = temp;
        mean += temp;
        esp_rom_delay_us(10);
    }
    jammerRead.lower = valueLOW;
    
    //High start range read
    __freq = 94310000;
    for(int c = 0; c <= 3;c++){
        BK_SetFreq(__freq + c * __freq_step);
        esp_rom_delay_us(50);
        temp = BK4819_GetRSSI();
        valueHIGH[c] = temp;
        mean += temp;
        esp_rom_delay_us(10);
    }

    __freq = 94500000;
    for(int c = 0; c <= 4;c++){
        BK_SetFreq(__freq + c * __freq_step);
        esp_rom_delay_us(50);
        temp = BK4819_GetRSSI();
        valueHIGH[4+c] = temp;
        mean += temp;
        esp_rom_delay_us(10);
    }
    jammerRead.mean = mean/15;
    jammerRead.upper = valueHIGH;
    return jammerRead;
}

uint8_t jammer_read_number = 0;
uint8_t jammer_calibrate(uint8_t mean){
    jammer_read_number++;
    jammer_rssi_calibrate[jammer_read_number] =  mean;
    ESP_LOGI(__func__,"jammer_read_number [%d]",jammer_read_number);
    uint8_t cont = 0;
    uint16_t avg = 0;
    while(cont <= jammer_read_number){
        avg += jammer_rssi_calibrate[cont];
        cont++;
    }
    ESP_LOGI(__func__,"avg verification = [%d]",avg);
    avg  = avg / jammer_read_number;

    if(jammer_read_number == 16){
        jammer_read_number = 1;
        memset(jammer_rssi_calibrate,0,15);
        jammer_rssi_calibrate[1] = avg;
    }
    return avg;
}
JammerPacket packet;
// le os valores de leitura do jammer, calcula uma media, coloca no array das leituras e depois atualiza o jammer floor
void jammer_routine(void){
    JammerRead jammerRead;
    jammerRead = Jammer_Detect();
    ESP_LOGI(__func__,"jammerRead: [%d]",jammerRead.mean);
    uint8_t avg = jammer_calibrate(jammerRead.mean);
    ESP_LOGI(__func__,"avg = [%d]",avg);
    uint16_t upper_sum_sqrdd = 0;
    uint16_t lower_sum_sqrdd = 0;
    for (int i = 0; i < 9; i++) {
        uint16_t diff = jammerRead.upper[i] - avg;
        upper_sum_sqrdd += diff*diff;
    }
    for (int i = 0; i < 6; i++) {
        uint16_t diff = jammerRead.lower[i] - avg;
        lower_sum_sqrdd += diff*diff;
    }
    uint16_t stddev = (upper_sum_sqrdd+lower_sum_sqrdd)/15;
    lower_sum_sqrdd = 0;
    upper_sum_sqrdd = 0;
    //ESP_LOGI(__func__,"stddev = [%d]",stddev);
    if(stddev > 10){
        ESP_LOGI(__func__,"jammer detectado [stddev = %d]",stddev);
    }
    packet.avg = avg;
    packet.stddev = stddev;
    xQueueSend(jammer,&packet,pdMS_TO_TICKS(5));
    graphic(21*(avg-30)/65);
    vTaskDelay(400/ portTICK_PERIOD_MS);
}