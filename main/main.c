#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <hd44780.h>
#include <esp_idf_lib_helpers.h>
#include <inttypes.h>
#include <stdio.h>
#include <esp_adc/adc_oneshot.h>
#include <driver/ledc.h>
#include <string.h>

#define dseat GPIO_NUM_4                            //Driver seat button pin
#define dbelt GPIO_NUM_6                            //Driver seatbelt button pin
#define pseat GPIO_NUM_5                            //Passenger seat button pin
#define pbelt GPIO_NUM_7                            //Passenger seatbelt button pin
#define transmission GPIO_NUM_3                     //Transmission button pin
#define gLED GPIO_NUM_13                            //Green LED pin
#define rLED GPIO_NUM_14                            //Red LED pin
#define alarm GPIO_NUM_12                           //Alarm pin
#define ModeSelector ADC_CHANNEL_1                  //Potentiometer pin for setting wiper mode
#define IntervalSelector ADC_CHANNEL_0              //Potentiometer pin for setting wiper interval
#define ADC_ATTEN ADC_ATTEN_DB_12                   //ADC Attenuation
#define BITWIDTH ADC_BITWIDTH_12                    //ADC Bitwidth
#define SHORT (1)                                   //Short delay for intermittent wipers
#define MEDIUM (3)                                  //Medium delay for intermittent wipers
#define LONG (5)                                    //Long delay for intermittent wipers
#define LEDC_TIMER LEDC_TIMER_0                     //Sets the LEDC timer
#define LEDC_MODE LEDC_LOW_SPEED_MODE               //Sets the LEDC speed mode
#define LEDC_OUTPUT_IO (16)                         //Define the output pin for LEDC
#define LEDC_CHANNEL LEDC_CHANNEL_0                 //Define LEDC channel
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT             //Set duty resolution to 13 bits
#define LEDC_FREQUENCY (50)                         //Set the PWM signal frequency in Hertz. 
#define LEDC_DUTY_MIN (200)                         //Set duty to move servo to 0 degrees
#define LEDC_DUTY_MAX (921)                         //Set duty to move servo to 180 degrees
#define LEDC_STOP (0)                               //Set duty to make servo stop
#define LEDC_DELAY (300/portTICK_PERIOD_MS)         //Define the delay needed for one 180 degree rotation


bool running = 1;                                             //Variable to track when car is running
bool reset = 1;                                               //Variable to track when the system has reset
bool error = 0;                                               //Variable for when the alarm should sound
bool ran = 1;                                                 //Variable to track if engine just started
int WiperMode = 0;                                            //Variable for setting the wiper mode
int WiperInterval = 0;                                        //Variable for setting the wipers' speed
int counterLOW = 0;                                           //Variable for delaying the LOW interval by 1 second
int counterMED = 0;                                           //Variable for delaying the LOW interval by 3 seconds 
int counterHIGH = 0;                                          //Variable for delaying the LOW interval by 5 seconds
char arr_modes[4][10] = {"OFF", "HIGH", "LOW", "INTERVAL"};   //List of mode options 
char arr_speeds[3][10] = {"SHORT", "MED", "LONG"};            //List of intermittent speed options

//Initialize functions for later
void config();
void print_status();
void welcome();
void run();
void gpio_isr_handler();
bool ready();
void WiperHandler();
void ledc_init();

void lcd(void *pvParameters){
    hd44780_t lcd =
    {
        .write_cb = NULL,
        .font = HD44780_FONT_5X8,
        .lines = 2,
        .pins = {
            .rs = GPIO_NUM_41,
            .e  = GPIO_NUM_37,
            .d4 = GPIO_NUM_36,
            .d5 = GPIO_NUM_35,
            .d6 = GPIO_NUM_48,
            .d7 = GPIO_NUM_47,
            .bl = HD44780_NOT_USED
        }
    };

    ESP_ERROR_CHECK(hd44780_init(&lcd));

    while(1) {
        hd44780_clear(&lcd);
        hd44780_gotoxy(&lcd, 0, 0);
        hd44780_puts(&lcd, "Mode: ");
        hd44780_puts(&lcd, arr_modes[WiperMode]);
        if (WiperMode == 3){
            hd44780_gotoxy(&lcd, 0, 1);
            hd44780_puts(&lcd, "Speed: ");
            hd44780_puts(&lcd, arr_speeds[WiperInterval]);
        }
        else {
            hd44780_gotoxy(&lcd, 0, 1);
            hd44780_puts(&lcd, "Speed: ");
        }
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
}


void app_main(void) {
    printf("RUNNING\n");
    config();
    ledc_init();

    //Handles Wiper Functions
    xTaskCreate(WiperHandler, "WiperHandler", 2048, NULL, 5, NULL);
    //Handles LCD functions
    xTaskCreate(lcd, "LCDmessages", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    //Configure ADC pins
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };                                                  // Unit configuration
    adc_oneshot_unit_handle_t adc1_handle;              // Unit handle
    adc_oneshot_new_unit(&init_config1, &adc1_handle);  // Populate unit handle

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = BITWIDTH
    };                                                  // Channel config
    adc_oneshot_config_channel                          // Configure the potentiometer channel
    (adc1_handle, ModeSelector, &config);

    adc_oneshot_config_channel                          // Configure the light sensor channel
    (adc1_handle, IntervalSelector, &config);
   
    adc_cali_curve_fitting_config_t cali_config = {     // Configure the potentiometer
        .unit_id = ADC_UNIT_1,
        .chan = ModeSelector,
        .atten = ADC_ATTEN,
        .bitwidth = BITWIDTH
    };

    adc_cali_handle_t adc1_cali_chan_handle;            // Calibration handle
    adc_cali_create_scheme_curve_fitting                // Populate cal handle
    (&cali_config, &adc1_cali_chan_handle);

    while(1) {                                          //Start the actual process
        if (reset == 1) {                               //Reset the system
            gpio_set_level(rLED, 0);
            welcome();
        }
        if (ready() == 1) {                             //Turn on green LED if all conditions met
            gpio_set_level(gLED, 1);
        } else {                                        //Turn off green LED if conditions not met
            gpio_set_level(gLED, 0);
        }
        while(running) {
            run();                                      //Run function for starting the car

            int mode_selector_adc_bits;                 //Variable for wiper mode potentiometer input in bits
            int mode_selector;                          //Variable for wiper mode potentiometer input in mV
            int int_selector_adc_bits;                  //Variable for wiper interval selector potentiometer input in bits
            int int_selector;                           //Variable for wiper interval selector potentiometer input in mV

            adc_oneshot_read                            //Get potentiometer input bits and make them mV
            (adc1_handle, ModeSelector, &mode_selector_adc_bits);
        
            adc_cali_raw_to_voltage
            (adc1_cali_chan_handle, mode_selector_adc_bits, &mode_selector);

            adc_oneshot_read                            //Get light sensor input bits and make them mV
            (adc1_handle, IntervalSelector, &int_selector_adc_bits);
        
            adc_cali_raw_to_voltage
            (adc1_cali_chan_handle, int_selector_adc_bits, &int_selector);


            //Sets wipers to the proper mode and interval based on the potentiometer readings
            if (mode_selector < 600) {
                WiperMode = 0;
            } else if (mode_selector < 1500 && mode_selector >= 600) {
                WiperMode = 1;
            } else if (mode_selector <2300 && mode_selector >= 1500) {
                WiperMode = 2;
            } else  if (mode_selector >= 2300) {
                WiperMode = 3;
            }

            if (int_selector < 1050) {
                WiperInterval = 0;
            } else if (int_selector < 2100 && int_selector >= 1050) {
                WiperInterval = 1;
            } else if (int_selector >= 2100) {
                WiperInterval = 2;
            }

            printf("%d --- ", WiperInterval);
            printf("%d  ///  ", int_selector);
            printf("%d --- ", WiperMode);
            printf("%d\n", mode_selector);
            vTaskDelay(20/portTICK_PERIOD_MS);
        }
        if (error == 1) {                               //Reset the system and sound the alarm
            print_status();
            gpio_set_level(alarm, 1);
            vTaskDelay(500/portTICK_PERIOD_MS);
            gpio_set_level(alarm, 0);
            error = 0;
            reset = 1;
        }
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
    vTaskDelay(20/portTICK_PERIOD_MS);
}

//Function for configuring all GPIO pins
void config(){
//Configure dseat pin
    printf("CONFIGGING\n");
    gpio_reset_pin(dseat);
    gpio_set_direction(dseat, GPIO_MODE_INPUT);
    gpio_pulldown_en(dseat);

    //Configure dbelt pin
    gpio_reset_pin(dbelt);
    gpio_set_direction(dbelt, GPIO_MODE_INPUT);
    gpio_pulldown_en(dbelt);

    //Configure pseat pin
    gpio_reset_pin(pseat);
    gpio_set_direction(pseat, GPIO_MODE_INPUT);
    gpio_pulldown_en(pseat);

    //Configure pbelt pin
    gpio_reset_pin(pbelt);
    gpio_set_direction(pbelt, GPIO_MODE_INPUT);
    gpio_pulldown_en(pbelt);

    //Configure transmission pin
    gpio_reset_pin(transmission);
    gpio_set_direction(transmission, GPIO_MODE_INPUT);
    gpio_pulldown_en(transmission);
    gpio_set_intr_type(transmission, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(transmission, gpio_isr_handler, NULL);
    gpio_intr_enable(transmission);

    //Configure gLED pin
    gpio_reset_pin(gLED);
    gpio_set_direction(gLED, GPIO_MODE_OUTPUT);

    //Configure rLED pin
    gpio_reset_pin(rLED);
    gpio_set_direction(rLED, GPIO_MODE_OUTPUT);

    //Configure alarm pin
    gpio_reset_pin(alarm);
    gpio_set_direction(alarm, GPIO_MODE_OUTPUT);
}

//Configured for active high
bool ready() {                                      //Define a function to check if all conditions are fufilled
    return gpio_get_level(dseat) 
    && gpio_get_level(dbelt) 
    && gpio_get_level(pseat) 
    && gpio_get_level(pbelt);
}

void print_status() {                               //Define a function for printing reason for car not starting
    if (gpio_get_level(dseat) == 0){                //Check if driver is seated
        printf("Driver Not Seated \n");             //Print if driver not seated
    }

    if (gpio_get_level(dbelt) == 0){                //Check if driver is buckled
        printf("Driver Not Buckled \n");            //Print if driver not buckled
    }

    if (gpio_get_level(pseat) == 0){                //Check if passenger is seated
        printf("Passenger Not Seated \n");          //Print if passenger not seated
    }

    if (gpio_get_level(pbelt) == 0){                //Check if passenger is buckled
        printf("Passenger Not Buckled \n");         //Print if passenger not buckled
    }
}

//Function for printing welcome message when driver sits
void welcome() {
    if (gpio_get_level(dseat)==1) {
        printf("Welcome to enhanced alarm system model 218-W25. \n");
        reset = 0;
    }

}

//Function for "starting the car"
void run() {
    if (ran == 1) {
        gpio_set_level(gLED, 0);
        gpio_set_level(rLED, 1);
        printf("Engine Started\n");
        ran = 0;
    }
}

//Interupt function for when transmission is pressed
void IRAM_ATTR gpio_isr_handler(void* arg) {
    if (ready() == 1 && running == 0) {             //Start engine if all conditions met
        running = 1;    
    } else if (running == 1) {                      //Stop engine if it is running
        reset = 1;
        ran = 1;
        running = 0;
    } else {                                        //Sound the alarm and print error messages if conditions not met
        error = 1;
    }
}

void WiperIntervalHandler(){
    if (WiperInterval == 0) {
        //Move the wipers back and forth after 1 second
        if (counterLOW == 50) {
            counterLOW = 0;
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MAX);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            vTaskDelay(LEDC_DELAY);
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MIN);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            vTaskDelay(LEDC_DELAY);
        }
        counterLOW++;
        //Stop the wipers
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_STOP);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    } else if (WiperInterval == 1) {
        //Move the wipers back and forth after 1 second
        if (counterMED == 150) {
            counterMED = 0;
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MAX);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            vTaskDelay(LEDC_DELAY);
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MIN);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            vTaskDelay(LEDC_DELAY);
        }
        counterMED++;
        //Stop the wipers
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_STOP);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);   
    } else if (WiperInterval == 2){
        //Move the wipers back and forth after 1 second
        if (counterHIGH == 250) {
            counterHIGH = 0;
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MAX);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            vTaskDelay(LEDC_DELAY);
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MIN);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            vTaskDelay(LEDC_DELAY);
        }
        counterHIGH++;
        //Stop the wipers
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_STOP);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    }
}

void WiperHandler() {
    while(1) {
        if (running) {
            if (WiperMode == 0) {
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_STOP);
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            } else if (WiperMode == 1) {
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MAX);
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
                vTaskDelay(LEDC_DELAY);
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MIN);
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
                vTaskDelay(LEDC_DELAY);
            } else if (WiperMode == 2) {
                counterLOW = 0;
                counterMED = 0;
                counterHIGH = 0;
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MAX);
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
                vTaskDelay(LEDC_DELAY);
                ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_MIN);
                ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
                vTaskDelay(LEDC_DELAY);
            } else if (WiperMode == 3) {
                WiperIntervalHandler();
            }
        }
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
}

void ledc_init()
{
    //Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  //Set output frequency at 50 Hz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    //Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0,                 //Set duty to 0%
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}
