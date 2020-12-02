

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
 * PB5 = Motor output Hex:0x20
 * ***************************************************************/









#include <dht_nonblocking.h> //library for dht11 temperature sensor
#define dht_sensor_type DHT_TYPE_11 //defining dht_sensor_type for DHT11 sensor


//DHT  Global Variables
static const int dht_sensor_pin = 27; //set PA6 as input for DHT sensor
DHT_nonblocking dht_sensor(dht_sensor_pin, dht_sensor_type); //initializing sensor

//PORT A register setup
volatile unsigned char* portA = (unsigned char*) 0x22;
volatile unsigned char* ddrA = (unsigned char*) 0x21;
volatile unsigned char* pinA = (unsigned char*) 0x20;

//PORT B register setup
volatile unsigned char* portB = (unsigned char*) 0x25;
volatile unsigned char* ddrB = (unsigned char*) 0x24;
volatile unsigned char* pinB = (unsigned char*) 0x23;

//Timer register setup
volatile unsigned char* myTCCR1A = (unsigned char*) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char*) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char*) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char*) 0x6F;
volatile unsigned int *myTCNT1 = (unsigned int*) 0x84; //This is timer1 counter lower bit
volatile unsigned char *myTIFR1 = (unsigned char*) 0x36;
volatile unsigned int *myOCR1A = (unsigned int*) 0x88;

//Global state booleans
volatile bool state = false; //on off state
volatile bool fan = false; //For polling on if the fan is on


//Prototype Functions
void off();
void on();
void idle();



void setup() {
  Serial.begin(9600);
  *ddrA |= 0x0F; //sets A0:A3 to output
  *ddrB |= 0x20; // sets PB5 to output
  *portA |= 0x10; //Initializes PA4 pullup resistor
 
  cli(); // stop interrupts
  *myTCCR1A &= 0x00; // set entire TCCR1A register to 0
  *myTCCR1B &= 0x00; // same for TCCR1B
  *myTCNT1 = 0; // initialize counter value to 0
  *myTCCR1B |= 0x80; //Sets CTC mode for timer interrupt
  *myOCR1A = 62499; // Set output compare interrupt to interrupt at 1 Hz
  *myTCCR1B |= 0x04; //Turns on Timer1
  *myTIMSK1 |= 0x02; //enable timer compare interrupt
  sei(); // allow interrupts
  off(); //disabled state
}


void loop(){
  float temperature;
  float humidity;
  if(state){ //if the system is on
    if(dht_sensor.measure(&temperature, &humidity ) == true){ //checks to see if sensor is operational, is successful will produce values
      Serial.println(temperature);
      if(temperature > 20){ // greater than 20 degrees celsius
        Serial.print( "T = " );
        Serial.print( temperature, 1 );
        Serial.print( " deg. C, H = " );
        Serial.print( humidity, 1 );
        Serial.println( "%" );
        fan_running(); //turns on fan
      }
    }
  }
}

ISR(TIMER1_COMPA_vect){
  if(!(*pinA & 0x10)){ //if button is pressed
    switch(state){
      case true: //if the system is on
        if(!(*pinA & 0x10)){ //leaves room for error on the button
          Serial.println("Turned off");
          off(); //Turns system off.
          break;
        }
      case false:
        if(!(*pinA & 0x10)){
          Serial.print("Loop turned on");
          on(); //Turns system on.
          break;
        } 
    }
  }
}

void on(){
  state = true;
  *portA &= 0xF0; //Turn off all LEDs
  *portA |= 0x02; //Turn on PA1
  
}

void off(){
  state = false;
  *portB &= 0xDF; //Turn off fan
  *portA &= 0xF0; //Turns off all LEDs
  *portA |= 0x01; //Turn on PA0
}

void fan_running(){
  *portA &= 0xF0; //Turn of all LEDs
  *portA |= 0x04; //Turn on PA2
  *portB |= 0x20; //Turn on PB5
}
