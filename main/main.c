#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"


#define dseat GPIO_NUM_4                            //Driver seat button pin
#define dbelt GPIO_NUM_6                            //Driver seatbelt button pin
#define pseat GPIO_NUM_5                            //Passenger seat button pin
#define pbelt GPIO_NUM_7                            //Passenger seat button pin
#define transmission GPIO_NUM_10                    //Passenger seat button pin
#define gLED GPIO_NUM_13                            //Green LED pin
#define rLED GPIO_NUM_14                            //Red LED pin
#define alarm GPIO_NUM_12                           //Alarm pin
#define headlights GPIO_NUM_40                      //Headlight Pin
#define selector ADC_CHANNEL_1                      //Potentiometer Pin
#define lsensor ADC_CHANNEL_7                       //Light Sensor Pin
#define ADC_ATTEN ADC_ATTEN_DB_12                   //ADC Attenuation
#define BITWIDTH ADC_BITWIDTH_12                    //ADC Bitwidth


bool running = 0;                                   //Variable to track when car is running
bool reset = 1;                                     //Variable to track when the system has reset
bool error = 0;                                     //Variable for when the alarm should sound
bool ran = 1;                                       //Variable to track if engine just started
bool autoLights = 0;                                //Variable to set the headlights to AUTO
bool on = 0;                                        //Variable to set the headlights to ON
int counterOn = 0;                                  //Counter used to turn headlights on after a 1sec delay
int counterOff = 0;                                 //Counter used to turn headlights off after a 2sec delay


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


void app_main(void) {

    //Configure dseat pin
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

    //Configure lHeadlight pin
    gpio_reset_pin(headlights);
    gpio_set_direction(headlights, GPIO_MODE_OUTPUT);

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
    (adc1_handle, selector, &config);

    adc_oneshot_config_channel                          // Configure the light sensor channel
    (adc1_handle, lsensor, &config);
   
    adc_cali_curve_fitting_config_t cali_config = {     // Configure the potentiometer
        .unit_id = ADC_UNIT_1,
        .chan = selector,
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
        while(running == 1) {
            run();                                      //Run function for starting the car

            int selector_adc_bits;                      //Variable for potentiometer input adc bits
            int selector_adc_mV;                        //Variable for potentiometer adc bits in mV
            int lsensor_adc_bits;                       //Variable for light sensor input adc bits
            int lsensor_adc_mV;                         //Variable for light sensor adc bits in mV

            adc_oneshot_read                            //Get potentiometer input bits and make them mV
            (adc1_handle, selector, &selector_adc_bits);
        
            adc_cali_raw_to_voltage
            (adc1_cali_chan_handle, selector_adc_bits, &selector_adc_mV);

            adc_oneshot_read                            //Get light sensor input bits and make them mV
            (adc1_handle, lsensor, &lsensor_adc_bits);
        
            adc_cali_raw_to_voltage
            (adc1_cali_chan_handle, lsensor_adc_bits, &lsensor_adc_mV);


            //Sets headlights to the proper mode based on the potentiometer readings
            if (selector_adc_mV < 1000) {
                autoLights = 1;
                on = 0;
            } else if (selector_adc_mV < 2200 && selector_adc_mV >= 1000) {
                autoLights = 0;
                on = 1;
            } else {
                on = 0;
                autoLights = 0;
            }

            //Handles AUTO headlights behaviour
            if (autoLights == 1) {
                if (lsensor_adc_mV >= 1500) {           //Turns headlights on after 1 second if dim outside
                    counterOff = 0;
                    if (counterOn >= 50) {
                        gpio_set_level(headlights, 1);
                        counterOn = 0;
                    }
                    counterOn++;
                } else {                                //Turns headlights off after 2 seconds if bright outside
                    counterOn = 0;
                    if (counterOff >= 100) {
                        gpio_set_level(headlights, 0);
                        counterOff = 0;
                    }
                    counterOff++;
                }
            } else if (on == 1) {                       //Turns headlights ON
                gpio_set_level(headlights, 1);
                counterOn = 0;
                counterOff = 0;
            } else {                                    //Turns headlights OFF
                gpio_set_level(headlights, 0);
                counterOn = 0;
                counterOff = 0;
            }
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
}