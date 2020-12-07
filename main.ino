


/*****************************************************************
 * Port A will control all 4 LEDS and sensor inputs.
 * PA0 = Yellow LED (Disabled) Hex: 0x01
 * PA1 = Green LED (Idle) Hex: 0x02
 * PA2 = Blue LED (Running) Hex: 0x04
 * PA3 = Red LED (Error) Hex: 0x08
 * PA4 = Button Input Hex: 0x10
 * PA5 = Temperature & Humidity Sensor Data Input Hex: 0x20
 * PA6 = Vent motor button input Hex: 0x40;
 * 
 * 
 * Port B will control motor and vent servo
 * PB5 = Motor output Hex:0x20
 * PB5 = Motor output Hex: 0x20
 * PB6 = Water Sensor power Hex: 0x40
 * 
 * 
 * 
 * Port F will control water sensor input
 * PF0 = Water sensor input Hex: 0x01
 * ***************************************************************/


#include <dht_nonblocking.h> //library for dht11 temperature sensor
#include <LiquidCrystal.h>
#include <Stepper.h> //library for stepper motor

#define dht_sensor_type DHT_TYPE_11 //defining dht_sensor_type for DHT11 sensor

//DHT  Global Variables
static const int dht_sensor_pin = 27; //set PA6 as input for DHT sensor
DHT_nonblocking dht_sensor(dht_sensor_pin, dht_sensor_type); //initializing sensor

//LCD Global Variables
LiquidCrystal lcd(46, 47, 48, 49, 50, 51);

//Water sensor Global Variables
volatile int water_level; //to measure water level

//Stepper Global Variables
const int stepsPerRevolution = 200;
Stepper myStepper(stepsPerRevolution, 30, 31, 32, 33);
volatile int step_count = 0;

//PORT A register setup
volatile unsigned char* portA = (unsigned char*) 0x22;
volatile unsigned char* ddrA = (unsigned char*) 0x21;
volatile unsigned char* pinA = (unsigned char*) 0x20;
//PORT B register setup
volatile unsigned char* portB = (unsigned char*) 0x25;
volatile unsigned char* ddrB = (unsigned char*) 0x24;
volatile unsigned char* pinB = (unsigned char*) 0x23;

//Timer1 register setup
volatile unsigned char* myTCCR1A = (unsigned char*) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char*) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char*) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char*) 0x6F;
volatile unsigned int *myTCNT1 = (unsigned int*) 0x84; //This is timer1 counter lower bit
volatile unsigned char *myTIFR1 = (unsigned char*) 0x36;
volatile unsigned int *myOCR1A = (unsigned int*) 0x88;

//Timer2 register setup
volatile unsigned char* myTCCR2A = (unsigned char*) 0xB0;
volatile unsigned char *myTCCR2B = (unsigned char*) 0xB1;
volatile unsigned char *myTIMSK2 = (unsigned char*) 0x70;
volatile unsigned int *myTCNT2 = (unsigned int*) 0xB2;
volatile unsigned char *myTIFR2 = (unsigned char*) 0x37;

//Setup analog to digital converter pointers
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;


//Global state booleans
volatile bool state = false; //on off state
volatile bool fan = false; //For polling on if the fan is on
volatile bool error = false; //for error state


//Prototype Functions
void off();
void on();
void idle();



void setup() {
  step_count = 0;
  
  Serial.begin(9600);
  lcd.begin(16, 2);

  adc_init();
  
  *ddrA |= 0x0F; //sets A0:A3 to output
  *ddrB |= 0x20; // sets PB5 to output
  *ddrB |= 0x60; //0b0010 sets PB5 to output and PB6 to output
  *portA |= 0x50; //Initializes PA4 and PA6 pullup resistor

  cli(); // stop interrupts
  *myTCCR1A &= 0x00; // set entire TCCR1A register to 0
  *myTCCR1B &= 0x00; // same for TCCR1B
  *myTCNT1 = 0; // initialize counter value to 0
  *myTCCR1B |= 0x80; //Sets CTC mode for timer interrupt
  *myTCCR1B |= 0x80; //Turns on CTC mode
  *myOCR1A = 62499; // Set output compare interrupt to interrupt at 1 Hz
  *myTCCR1B |= 0x04; //Turns on Timer1
  *myTIMSK1 |= 0x02; //enable timer compare interrupt
  sei(); // allow interrupts

 myStepper.setSpeed(60);
   
  off(); //disabled state

}



void loop(){
  float temperature_c;
  float temperature_f;
  float humidity;
  int steps = 0;
  if(state){ //if the system is on
    if(!(*pinA & 0x40)){
      mydelay(100);
      if(step_count < 3){
        myStepper.step(stepsPerRevolution);
        step_count++;
      }
      else{
        step_count = 0;
        steps = (stepsPerRevolution*3);
        myStepper.step(-steps);
      }
      mydelay(100);
    }
    if(!error){ //if there is no error
      if(measure_environment(&temperature_c, &humidity ) == true){ //checks to see if sensor is operational, is successful will produce values
        temperature_f = tempc_tempf(temperature_c);
        lcd.setCursor(0,0);
        lcd.print( "T = " );
        lcd.print( temperature_f );
        lcd.print( " deg. F");
        lcd.setCursor(0,1);
        lcd.print("H = " );
        lcd.print( humidity );
        lcd.print( "%" );
        
        if(temperature_c >= 22){ // greater than 20 degrees celsius
          if(!fan){
            fan_on(); //turns on fan
          }
        }
        else{
          if(fan){
            fan_off();
          }
        }
      }
    }
  }
}




ISR(TIMER1_COMPA_vect){
  if(!(*pinA & 0x10)){ //if button is pressed
      switch(state){ //Check system state
        case true:
          if(!(*pinA & 0x10)){
            setup();
            break;
          }
        case false:
          if(!(*pinA & 0x10)){
            Serial.println("System turned on");
            on(); //Turns on system
            break;
          } 
      }
  }
  if(state){ //if system is on
    water_level = adc_read(0); //get data from PF0
    if(water_level < 100){
      if(!error){
        error_on(); //turn on error state
      }
    }
    else{
      error = false; //turn off error state
      if(!fan){
        *portA &= 0xF0;
        *portA |= 0x02;
      }
    }
  }
}



void adc_init(){
  //set A register
  *my_ADCSRA |= 0x80; //enable ADC
  *my_ADCSRA &= 0xDF; //disable ADC trigger mode
  *my_ADCSRA &= 0xF7; //disable ADC interrupt
  *my_ADCSRA &= 0xF8; //set prescaler

  //set B register
  *my_ADCSRB &= 0xF7; //reset the channel
  *my_ADCSRB &= 0xF8; //set free running mode

  //set mux register
  *my_ADMUX &= 0x7F; //clear bit 7 for AVCC reference
  *my_ADMUX |= 0x40; //set bit 6 for AVCC reference
  *my_ADMUX &= 0xDF; //set bit 5 for right adjusted result
  *my_ADMUX &= 0xE0; //reset channel
}


unsigned int adc_read(unsigned char adc_channel){
  //clear channel bits
  *my_ADMUX &= 0xE0;
  *my_ADCSRB &= 0xF7;

  //set channel
  if(adc_channel > 7 ){
    adc_channel -= 8; 
    *my_ADCSRB |= 0x08; //sets MUX5 if > 7
  }
  *my_ADMUX |= adc_channel;

  //starts conversion
  *my_ADCSRA |= 0x40;
  while((*my_ADCSRA & 0x40) != 0);

  return *my_ADC_DATA;
}


void mydelay(unsigned int freq){
  //stops the timer
  *myTCCR2B &= 0x00;
  // set the counts
  *myTCNT2 = (unsigned int) (256 - freq);
  // start the timer
  *myTCCR2B |= 0x04;
  // wait for overflow
  while((*myTIFR2 & 0x01)==0){};
  // stop the timer
  *myTCCR2B &= 0x00;
  // reset TOV
  *myTIFR2 |= 0x01;
}



static bool measure_environment( float *temperature, float *humidity ){
  static unsigned long measurement_timestamp = millis( );

  /* Measure once every four seconds. */
  if( millis( ) - measurement_timestamp > 3000ul )
  {
    if( dht_sensor.measure( temperature, humidity ) == true )
    {
      measurement_timestamp = millis( );
      return( true );
    }
  }

  return( false );
}


float tempc_tempf(float tempc){
  float tempf = (((tempc*9)/5) + 32);
  return tempf;
}

void on(){
  state = true;
  *portA &= 0xF0; //Turn off all LEDs
  *portA |= 0x02; //Turn on PA1
  *portB |= 0x40; //Turn on PB6
}


void off(){
  state = false;
  error = false;
  fan = false;
  *portB &= 0xDF; //Turn off fan
  *portA &= 0xF0; //Turns off all LEDs
  *portB &= 0x9F; //Turn off motor and water sensor
  *portA |= 0x01; //Turn on PA0
}


void fan_on(){
  fan = true;
  Serial.println("Motor on");
  *portA &= 0xF0; //Turn off all LEDS
  *portA |= 0x04; //Turn on PA2
  *portB |= 0x20; //Turn on PB5
}


void fan_off(){
  fan = false;
  Serial.println("Motor off");
  *portB &= 0xDF; //Turn off fan
  *portA &= 0xF0; //Turn off all LEDs
  *portA |= 0x02; //Turn on PA1
}

void error_on(){
  error = true;
  fan = false;
  *portB &= 0xDF; //Turn off fan
  *portA &= 0xF0; //Turn off all LEDs
  *portA |= 0x08; //Turn on PA3
  lcd.setCursor(0,0);
  lcd.print("ERROR: ADD WATER");
  lcd.setCursor(0,1);
  lcd.print("          ");
}
