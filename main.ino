

/*****************************************************************
 * Port A will control all 4 LEDS and sensor inputs.
 * PA0 = Yellow LED (Disabled) Hex: 0x01
 * PA1 = Green LED (Idle) Hex: 0x02
 * PA2 = Blue LED (Running) Hex: 0x04
 * PA3 = Red LED (Error) Hex: 0x08
 * PA4 = Button Input Hex: 0x10
 * PA5 = Temperature & Humidity Sensor Data Input Hex: 0x20
 * 
 * 
 * 
 * Port B will control motor and vent servo
 * PB5 = Motor output Hex: 0x20
 * PB6 = Water Sensor power Hex: 0x40
 * 
 * 
 * 
 * Port F will control water sensor input
 * PF0 = Water sensor input Hex: 0x01
 * ***************************************************************/









#include <dht_nonblocking.h> //library for dht11 temperature sensor
#define dht_sensor_type DHT_TYPE_11 //defining dht_sensor_type for DHT11 sensor


//DHT  Global Variables
static const int dht_sensor_pin = 27; //set PA6 as input for DHT sensor
DHT_nonblocking dht_sensor(dht_sensor_pin, dht_sensor_type); //initializing sensor

int analogPin = A0; //PF0

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

//Global state booleans
volatile bool state = false; //for disabled state
volatile bool fan = false; //for running state
volatile bool error = false; //for error state

//Global water level int
volatile int water_level; //to measure water level



void setup() {
  Serial.begin(9600);
  *ddrA |= 0x0F; //sets A0:A3 to output
  *ddrB |= 0x60; //0b0010 sets PB5 to output and PB6 to output
  *portA |= 0x10; //Initializes PA4 pullup resistor
 
  cli(); // stop interrupts
  *myTCCR1A &= 0x00; // set entire TCCR1A register to 0
  *myTCCR1B &= 0x00; // same for TCCR1B
  *myTCNT1 = 0; // initialize counter value to 0
  *myTCCR1B |= 0x80; //Turns on CTC mode
  *myOCR1A = 62499; // Set output compare interrupt to interrupt at 1 Hz
  *myTCCR1B |= 0x04; //Turns on Timer1
  *myTIMSK1 |= 0x02; //enable timer compare interrupt
  sei(); // allow interrupts
  
  off(); //Disabled state
}


void loop(){
  float temperature_c;
  float temperature_f;
  float humidity;
  
  if(state){ //If system is on
    if(!error){ //While no error
       if(dht_sensor.measure(&temperature_c, &humidity ) == true){ //See if temperature sensor is working
        
          temperature_f = tempc_tempf(temperature_c); //Convert temperature from C to F
          
          if(temperature_c > 23){
            Serial.print( "T = " );
            Serial.print( temperature_f, 1 );
            Serial.print( " deg. F, H = " );
            Serial.print( humidity, 1 );
            Serial.println( "%" );
            fan_on(); //Turn on fan
          }
          else{
            fan_off(); //Turn off fan
          }
      }
    }
  }
}


ISR(TIMER1_COMPA_vect){
  if(!(*pinA & 0x10)){
    switch(state){ //Check system state
      case true:
        if(!(*pinA & 0x10)){
          Serial.println("Turned off");
          off(); //Turns off system
          break;
        }
      case false:
        if(!(*pinA & 0x10)){
          Serial.println("Turned on");
          on(); //Turns on system
          break;
        } 
    }
  }
  if(state){ //if system is on
    water_level = analogRead(analogPin); //get data from PF0
    if(water_level < 100){
      error_on(); //turn on error state
      Serial.println("ERROR: ADD WATER");
    }
    else{
      error = false; //turn off error state
    }
  }
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
  *portB &= 0x9F; //Turn off motor and water sensor
  *portA &= 0xF0; //Turn of all LEDS
  *portA |= 0x01; //Turn on PA0
}


void fan_on(){
  fan = true;
  *portA &= 0xF0; //Turn off all LEDS
  *portA |= 0x04; //Turn on PA2
  *portB |= 0x20; //Turn on PB5
}


void fan_off(){
  fan = false;
  *portB &= 0xDF; //Turn off fan
  *portA &= 0xF0; //Turn off all LEDs
  *portA |= 0x02; //Turn on PA1
}

void error_on(){
  error = true;
  *portB &= 0xDF; //Turn off fan
  *portA &= 0xF0; //Turn off all LEDs
  *portA |= 0x08; //Turn on PA3
}
