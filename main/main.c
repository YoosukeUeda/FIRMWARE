#include "pinout.h"
#include "jammer.c"
#include "batery.c"

typedef struct {
    uint16_t data;
        uint16_t max;
    uint32_t frequency;
    uint8_t batery;
} DataPacket;


static volatile bool is_debouncing = false;
uint32_t frequency = 43490000;
int Button_HIGH = 0;
int Button_LOW = 0;
static esp_timer_handle_t debounce_timer;
QueueHandle_t piruli;
QueueHandle_t jammer;
uint16_t test2 = 0;
uint32_t test = 0;

void lcd(){
    pin_init();
	ST7920_Init();
  	uint8_t font = FONTID_6X8M;
	uint8_t font2 = FONTID_10X16F;
    ST7920_Clear();
	ST7920_Clear();
	ST7920_GraphicMode(1);
	while(1)
	{
        DataPacket packet;
        JammerPacket Jpacket;
        if(xQueueReceive(piruli,&packet,pdMS_TO_TICKS(5))){
            packet.data = 45*packet.data/360;
            char str[12];
            sprintf(str,"%ddB",packet.data);
            disp1color_printf(6,15,font2,"      ");
            disp1color_printf(6,15,font2,str);
            disp1color_printf(67,4,font,"Bat. 100%%");
            float inter  = (float)packet.frequency/100000;
            sprintf(str,"%.1fMHz",inter);
            disp1color_printf(67,25,font,str);

            packet.max = 45*packet.max/360;
            sprintf(str,"Max:%ddB",packet.max);
            disp1color_printf(6,6,font,"        ");
            disp1color_printf(6,6,font,str);
            DrawRectangle(0,0,60,36);
            DrawRectangle(60, 0, 67, 18);
            DrawRectangle(60, 18, 67, 18);
        }
        if(xQueueReceive(jammer,&Jpacket,pdMS_TO_TICKS(5))){
            char str[20];
            sprintf(str,"avg:%d",Jpacket.avg);
            disp1color_printf(6,15,font2,"       ");
            disp1color_printf(6,15,font2,str);
            disp1color_printf(62,4,font,"Bat. 100&");
            sprintf(str,"StdDev:%d",Jpacket.stddev);
            disp1color_printf(62,25,font,"           ");
            disp1color_printf(62,25,font,str);
            disp1color_printf(6,6,font,"JAMMER");

            DrawRectangle(0,0,60,36);
            DrawRectangle(60, 0, 67, 18);
            DrawRectangle(60, 18, 67, 18);
        }
		disp1color_UpdateFromBuff();
		vTaskDelay(25/portTICK_PERIOD_MS);
	}
}

void BK_Setup()
{
    vTaskDelay(200 / portTICK_PERIOD_MS);
    BK4819_SetFrequency(frequency);// seta frequência
    vTaskDelay(200 / portTICK_PERIOD_MS);
    BK4819_RX_TurnOn();// liga rx
    vTaskDelay(200 / portTICK_PERIOD_MS);
    BK4819_WriteRegister(BK4819_REG_3F, 0xFFFF); // não lembro, tem que olhar o datasheet
    vTaskDelay(200 / portTICK_PERIOD_MS);
    BK4819_WriteRegister(BK4819_REG_02, 0); // não lembro, tem que olhar o datasheet
    vTaskDelay(200 / portTICK_PERIOD_MS);
    BK4819_SetAF(BK4819_AF_OPEN);// liga áudio'
    vTaskDelay(200 / portTICK_PERIOD_MS);
}


// Interrupt service routine (ISR)
static void button_handler(void* arg)
{
    if (!is_debouncing) {
        uint32_t gpio_num = (uint32_t) arg;
        if(gpio_num == PIN_ADVANCE){   
            frequency += 10000;
            if(frequency > 43490000)
                frequency = 4330000;
            BK4819_SetFrequency(frequency);
            is_debouncing = true;
            esp_timer_start_once(debounce_timer, DEBOUNCE_TIME_US);
        }else if(gpio_num == PIN_RETURN){
            frequency -= 10000;
            if(frequency < 4330000)
                frequency = 43490000;
            BK4819_SetFrequency(frequency);
            is_debouncing = true;
            esp_timer_start_once(debounce_timer, DEBOUNCE_TIME_US);
        }
    }
}
static void Button_loop(){
    int button1 = 0;
    int button2 = 0;
    while(1){
        vTaskDelay(20/portTICK_PERIOD_MS);
        button1 = gpio_get_level(PIN_ADVANCE);
        button2 = gpio_get_level(PIN_RETURN);
        if(button1){
            button_handler((void *)PIN_ADVANCE);
        }else if(button2){
            button_handler((void *)PIN_RETURN);
        }
    }
}
static void debounce_timer_callback(void* arg)
{
    is_debouncing = false;
}

void bk_main(){
    int StateRadio = 0;
    uint16_t RegValue = 0;
    DataPacket packet;
    packet.max = 0;
    packet.data = 0;
    packet.frequency = 0;
    uint8_t cnt_sum = 0;
    uint32_t intermediate_data;
    uint32_t sum_data;
    while (1) {
        vTaskDelay(10 / portTICK_PERIOD_MS);	
        switch (StateRadio){
            case 0:
                BK4819_Init();
                //BK4819_Init3(); // pode ser este também
                vTaskDelay(100 / portTICK_PERIOD_MS);	
                RegValue = BK4819_ReadRegister(0x00);// testa comunicação com chip
                if (RegValue == 0x4819){
                    StateRadio++;
                }
                ESP_LOGI(__func__,"case 0 %d",RegValue); 
                break;
            case 1:
                
                BK_Setup();
                //ESP_LOGI(__func__,"INICIANDO TESTES FREQ");
                //vTaskDelay(2000/portTICK_PERIOD_MS);
                
                //for(int freq = 43300000;freq <= 43490000; freq+=10000){
                //    BK_SetFreq(freq);
                //}
                //vTaskDelay(3000/portTICK_PERIOD_MS);
                ESP_LOGI(__func__,"INICIANDO JAMMER DETECTOR");
                vTaskDelay(2000/portTICK_PERIOD_MS);
                Jammer_Detect();
                vTaskDelay(2000/portTICK_PERIOD_MS);

				StateRadio++;
                BK_SetFreq(43490000);
                ESP_LOGI(__func__,"case 1");
                break;
			case 2: // mudar dps
                intermediate_data = BK4819_GetRSSI();
                
                //move_graphic();
                sum_data += intermediate_data;

                    if(packet.max < intermediate_data){
                    packet.max = intermediate_data;
                }
                if(cnt_sum%5 == 0 && cnt_sum != 15){
                    xQueueSend(piruli,&packet,pdMS_TO_TICKS(5));
                    graphic(21*(packet.max-30)/150);
                    packet.max = 0;
                }
                if(++cnt_sum == 15){
                    packet.frequency = Read_BK4819_Frequency();
                    packet.data = (uint16_t)sum_data/cnt_sum;
                    graphic(21*(packet.max-30)/150);
                    packet.max = 0;
                    xQueueSend(piruli,&packet,pdMS_TO_TICKS(5));
                    ESP_LOGI(__func__,"rssi=%d",packet.data);
                    ESP_LOGI(__func__,"frequencia no chip: %ld",packet.frequency);
                    ESP_LOGI(__func__,"frequencia no codigo: %ld",frequency);
                    cnt_sum = 0;
                    sum_data = 0;
                }
                vTaskDelay(5 / portTICK_PERIOD_MS);	
                break;
            case 3:
                jammer_routine();
                
	    }
    }
}

void app_main()
{   
    jammer = xQueueCreate(2, sizeof(DataPacket));
    piruli = xQueueCreate(2, sizeof(DataPacket));
    ESP_LOGI(__func__,"Iniciando display");
    xTaskCreate(&lcd,"lcd",2048,NULL,2,NULL);
    const esp_timer_create_args_t debounce_timer_args = {
        .callback = &debounce_timer_callback,
        .name = "debounce_timer"
    };
    xTaskCreate(&bk_main,"bk4819",2048,NULL,1,NULL);

    //xTaskCreate(&Button_loop,"button_loop",2048,NULL,1,NULL);
    esp_timer_create(&debounce_timer_args, &debounce_timer);
    
    
    setup_PIN();
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,        // Desativa interrupções
        .mode = GPIO_MODE_INPUT,              // Configura o pino como saída
        .pin_bit_mask = (1ULL << PIN_ADVANCE), // Configura o pino específico
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Desativa pull-down
        .pull_up_en = GPIO_PULLUP_DISABLE      // Desativa pull-up
    };
    gpio_config(&io_conf);
    gpio_config_t io_conf2 = {
        .intr_type = GPIO_INTR_POSEDGE,        // Desativa interrupções
        .mode = GPIO_MODE_INPUT,              // Configura o pino como saída
        .pin_bit_mask = (1ULL << PIN_RETURN), // Configura o pino específico
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Desativa pull-down
        .pull_up_en = GPIO_PULLUP_DISABLE      // Desativa pull-up
    };
    gpio_config(&io_conf2);
    while(1){
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}