

/*****************************************************************
 * Port A will control all 4 LEDS.
 * PA0 = Yellow LED (Disabled)
 * PA1 = Green LED (Idle)
 * PA2 = Blue LED (Running)
 * PA3 = Red LED (Error)
 * PA4 = Button Input (Changes from any state to disabled or from disabled to idle)
 * 
 * 
 * 
 * Port B will control motor and vent servo
 * PB5 = Motor output
 * ***************************************************************/









//PORT A register setup
volatile unsigned char* portA = (unsigned char*) 0x22;
volatile unsigned char* ddrA = (unsigned char*) 0x21;
volatile unsigned char* pinA = (unsigned char*) 0x20;

//PORT B register setup
volatile unsigned char* portB = (unsigned char*) 0x25;
volatile unsigned char* ddrB = (unsigned char*) 0x24;
volatile unsigned char* pinB = (unsigned char*) 0x23;

//Global state booleans
volatile bool state = false;

//Prototype Functions
void off();
void on();
void idle();


void setup() {
  *ddrA |= 0x0F; //sets A0:A3 to output
  *ddrB |= 0x20; //0b0010 sets PB5 to output
  Serial.begin(9600);
  *portA |= 0x10; //Initializes PA4 pullup resistor
  off();
  idle();
}

void loop() {
  if(state){
    if(!(*pinA & 0x10)){
      Serial.println("Turned off");
      off();
    }
  }
  delay(500);
  if(!state){
    if(!(*pinA & 0x10)){
      Serial.print("Loop turned on");
      on();
     }
  }
  delay(500);
}//end loop

void idle(){
  while(state){
    if(!(*pinA & 0x10)){
      Serial.println("Turned on");
      on();
    }
  }
}

void on(){
  state = true;
  *portA &= 0xF0;
  *portA |= 0x02;
  *portB |= 0x20;
}

void off(){
  state = false;
  *portB &= 0xDF;
  *portA &= 0xF0;
  *portA |= 0x01;
}