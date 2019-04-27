/*---------- Library ------------------*/
#include "vl53l0x/pic16f886.h"
#include "vl53l0x/VL53L0X.h"

//#include <math.h>

#define PIN_GATE_12V PIN_B2
#define PIN_BUTTON_YELLOW PIN_B3
#define PIN_BUTTON_BLACK PIN_B4
#define PIN_BUTTON_RED PIN_B5
#define PIN_LED PIN_B6
#define FAN_MAX_PWM 1023
#define ON 1
#define OFF 0
unsigned int16 couter = 0;
unsigned int8 button = 0;
unsigned int8 buttonPrev = 0;

int1 status = OFF;
/*---------- END Library ------------------*/

/*----------- Timer ---------------*/
#define millis() (msTimer)
unsigned int32 msTimer = 0;
/*----------- END Timer ---------------*/

#INT_TIMER1
void timer1_isr()
{
   set_timer1(65286); // keep period at 1 ms (at 8 MHz)
   msTimer++;
}

/*------------- Setup Laser -------------*/

// Uncomment this line to use long range mode. This
// increases the sensitivity of the sensor and extends its
// potential range, but increases the likelihood of getting
// an inaccurate reading because of reflections from objects
// other than the intended target. It works best in dark
// conditions.

#define LONG_RANGE

// Uncomment ONE of these two lines to get
// - higher speed at the cost of lower accuracy OR
// - higher accuracy at the cost of lower speed

#define HIGH_SPEED
//#define HIGH_ACCURACY

/*-------------- END Setup Timer ------------*/

/*------------------ 7-Segments --------------------*/
#define SEGMENTS_SLAVE_ADDRESS 0xB0

#define REGISTER_SIZE 8

#define DISTANCT_MIN 157
#define DISTANCT_MAX 20
/*--------------- END 7-Segments -------------------*/

/*============= Global Variable ===================*/
#define SENSOR_SENDER_TIME_MS 10
unsigned int32 BALL_FALLING_TIME_MS = 10;

unsigned int32 ball_lasttime = 0;
unsigned int32 ball_lasttime_before = 0;
unsigned int32 sensor = 0;

unsigned int32 sensor_time_sender = 0;
unsigned int32 ball_fall_time_prev = 0;

unsigned int32 ball_height = 90;

/*=============== END GLOABAL VARIABLE ================*/

/* ++++++++++++++++++ SETUP FUNCTION ++++++++++++++++*/
int8 getInput()
{
   int8 g = input(PIN_BUTTON_RED) << 2 | input(PIN_BUTTON_BLACK) << 1 | input(PIN_BUTTON_YELLOW);
   return g;
}

void press2Start()
{
   while (1)
   {
      if (input(PIN_BUTTON_YELLOW))
      {
         break;
      }
   }
}

#INT_RB
void rb_isr(void)
{
   button = getInput();
}
/* ++++++++++++++++++ SETUP FUNCTION ++++++++++++++++*/

/*============== FUNCTION DEFINE ================*/

void pic_setup()
{
   setup_adc_ports(sAN0); // setup PIN A0 as analog input
   set_adc_channel(0);
   setup_adc(ADC_CLOCK_INTERNAL);
   delay_ms(2000); // Wait Gogo Display
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_8);
   set_timer1(65286); // keep period at 1 ms (at 8 MHz)
   enable_interrupts(INT_TIMER1);
   enable_interrupts(INT_RB3); // generate interrupt when B7 changes
   enable_interrupts(INT_RB4); // generate interrupt when B7 changes
   enable_interrupts(INT_RB5); // generate interrupt when B7 changes
   enable_interrupts(GLOBAL);

   // # Setup Hardware-PWM @pin13 (cc1)
   // # Period = 1ms , freq = 1kHz
   setup_ccp1(ccp_pwm);
   setup_timer_2(T2_DIV_BY_16, 124, 8);
   set_pwm1_duty((INT16)0);
}

void show(int number)
{
   i2c_start();
   i2c_write(SEGMENTS_SLAVE_ADDRESS);
   i2c_write(2);             // set REGISTER pointer to location 2
   i2c_write(number >> 8);   // send high byte
   i2c_write(number & 0xff); // send low byte
   i2c_stop();
}


void fan(int1 sw)
{
   if (sw)
   {
      set_pwm1_duty((INT16)FAN_MAX_PWM);
   }
   else
   {
      set_pwm1_duty(0);
   }
}

void laser_print()
{
   sensor = readRangeSingleMillimeters();
   if (millis() - sensor_time_sender >= SENSOR_SENDER_TIME_MS)
   {
      printf("%lu\r\n", sensor);
      if (timeoutOccurred())
      {
         printf(" TIMEOUT\r\n");
      }
      show(sensor);
   }
}

void laser()
{
   sensor = readRangeSingleMillimeters();
}


void shoot()
{
   fan(OFF);
   delay_ms(5000);
   fan(ON);
   delay_ms(2700);
}

void buttonCommand(){
if (button == 4)
   {
      if (buttonPrev != 4)
      {
         ball_height += 10;
         show(ball_height);
         printf("%Lu\n",ball_height);
         buttonPrev = 4;
      }
   }
   else if (button == 2)
   {
      if (buttonPrev != 2)
      {
         ball_height -= 10;
         show(ball_height);
        printf("%Lu\n",ball_height);
         buttonPrev = 2;
      }
   }
   else if (button == 1)
   {
      ball_height = 70;
      show(ball_height);
      printf("%Lu\n",ball_height);
   }
   else
   {
      buttonPrev = 0;
   }

   if (ball_height < 20)
   {
      ball_height = 20;
   }

   if (ball_height > 160)
   {
      ball_height = 150;
   }
}
void getData()
{
   laser();

   if (sensor >= ball_height)
   {
      fan(ON);
      output_high(PIN_B6);
      ball_fall_time_prev = millis();
 
   }
   else
   {
      if (millis() - ball_fall_time_prev >= BALL_FALLING_TIME_MS)
      {
         output_low(PIN_B6);
         fan(OFF);

      }
      
   }












































   buttonCommand();

   
}


/*============== END FUNCTION DEFINE ================*/

void setup()
{
   pic_setup();
   setup_adc_ports(sAN0); // setup PIN A0 as analog input
   setup_adc(ADC_CLOCK_INTERNAL);

   init();
   setTimeout(200);
#if defined LONG_RANGE

   // lower the return signal rate limit (default is 0.25 MCPS)
   setSignalRateLimit(0.1);
   // increase laser pulse periods (defaults are 14 and 10 PCLKs)
   setVcselPulsePeriod(VcselPeriodPreRange, 18);
   setVcselPulsePeriod(VcselPeriodFinalRange, 14);
#endif

#if defined HIGH_SPEED
   // reduce timing budget to 20 ms (default is about 33 ms)
   setMeasurementTimingBudget(20000);
#elif defined HIGH_ACCURACY
   // increase timing budget to 200 ms
   setMeasurementTimingBudget(200000);
#endif

  
}

void loop()
{
   getData();
}

void main()
{
   setup();
   show(111);
   delay_ms(200);
   shoot();
   
   while (1)
   {
      getData();
   }

}

#include "VL53L0X/VL53L0X.c"
