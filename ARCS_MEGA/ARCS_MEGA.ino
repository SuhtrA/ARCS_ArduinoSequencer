/* ARCS (Arduino Random Control-voltage Sequencer) - Modular Sequencer
  ---------------
  Author : Arthus Touzet, Aix-Marseille Université

  Arduino code for an 8 step 3 tracks sequencer providing gate output signal with a duty defined by the user.
  This code is part of an academic student research about modular systems and ATMega implementation.
  This work is licensed under the GNU General Public Licence wich allows any type of use of this code except distributing closed source versions.

  ---------------

  09.01.2023 First edit of this code
  01.02.2023 Update of the code for Arduino MEGA 2560 board

  ---------------

  This code requires Arduino Mega 2560 to be functionnal.

  Hardware :
  A 16 channels multiplexer/demultiplexer CD74HC4067
  A KY - 040 rotary button
  A NeoPixel Ring 16
  8 10k potentiometers
  5 push buttons

  ---------------

/* 
██╗███╗   ██╗ ██████╗██╗     ██╗   ██╗██████╗ ███████╗
██║████╗  ██║██╔════╝██║     ██║   ██║██╔══██╗██╔════╝
██║██╔██╗ ██║██║     ██║     ██║   ██║██║  ██║█████╗  
██║██║╚██╗██║██║     ██║     ██║   ██║██║  ██║██╔══╝  
██║██║ ╚████║╚██████╗███████╗╚██████╔╝██████╔╝███████╗
╚═╝╚═╝  ╚═══╝ ╚═════╝╚══════╝ ╚═════╝ ╚═════╝ ╚══════╝ 
*/
//libraries

#include <RotaryEncoder.h> 
#include <ezButton.h>
#include <MIDI.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>


/* 
██████╗ ███████╗███████╗██╗███╗   ██╗███████╗
██╔══██╗██╔════╝██╔════╝██║████╗  ██║██╔════╝
██║  ██║█████╗  █████╗  ██║██╔██╗ ██║█████╗  
██║  ██║██╔══╝  ██╔══╝  ██║██║╚██╗██║██╔══╝  
██████╔╝███████╗██║     ██║██║ ╚████║███████╗
╚═════╝ ╚══════╝╚═╝     ╚═╝╚═╝  ╚═══╝╚══════╝ 
*/
//___________________________________________

  //input/output initialisation
  #define MIDIinPin 0
  #define MIDIoutPin 1

 // MIDI_CREATE_DEFAULT_INSTANCE();

  //#define clockInPin 2 //pin 2 is used to handle an interruption to fire steps from external clock
  #define ioPin 2 //pin 3 is used to handle an interruption to stop all process
  #define gateOut1 30
  
  //buttons
  ezButton buttonLeft(23);
    ezButton buttonRight(25);
      ezButton buttonRST(27);
  

  // binary pins to communicate with 74HC4067 multiplexer
  #define s0Mux 36
  #define s1Mux 38
  #define s2Mux 40
  #define s3Mux 42

  //CV outputs are set to timer 4 PWM pins
  #define cvOut1 6
  #define cvOut2 7
  #define cvOut3 8

  #define neoPixelPin 10 //pwm pin for the NeoPixel Ring
  #define NUMPIXELS 16 //number of leds in the NeoPixel ring


  #define pinAKY  49  //CLK Output A of the encoder
  #define pinBKY  47 //DT Output B of the encoder
  ezButton button1(46); // switch of the encoder



  //#define bpmPot A0 //tempo control
  #define sigMux A0 // analog pin for 74HC4067 multiplexer
  #define seedPin A1 //analog pin used for the Von Neumann algorithm

  #define screenSDA A4 //OLED i2C SDA Pin
  #define screenSCL A5 //OLED i2C SCL 
  
  //dimensions de l'écran OLED https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* 
██╗   ██╗ █████╗ ██████╗ ██╗ █████╗ ██████╗ ██╗     ███████╗███████╗
██║   ██║██╔══██╗██╔══██╗██║██╔══██╗██╔══██╗██║     ██╔════╝██╔════╝
██║   ██║███████║██████╔╝██║███████║██████╔╝██║     █████╗  ███████╗
╚██╗ ██╔╝██╔══██║██╔══██╗██║██╔══██║██╔══██╗██║     ██╔══╝  ╚════██║
 ╚████╔╝ ██║  ██║██║  ██║██║██║  ██║██████╔╝███████╗███████╗███████║
  ╚═══╝  ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝╚══════╝╚══════╝ 
*/
//__________________________________________________________________
  // On/Off state
    bool ioState = false;

  // Rotary button
    // Setup a RotaryEncoder with 4 steps per latch for the 2 signal input pins: (from SimplePollRotator exemple of OtaryEncoder librairy)
    RotaryEncoder encoder(pinAKY, pinBKY, RotaryEncoder::LatchMode::FOUR3); 
  // Neopixel Init
    Adafruit_NeoPixel pixels(NUMPIXELS, neoPixelPin, NEO_GRB + NEO_KHZ800);

    
  //tempo parameter
    int newBpm = 60;  //next tempo value to be set
    int bpm = 60; //tempo value

  //step parameter
    byte globalStepCount = 0;
    byte oldStep[3] = {0};
    byte stepCount[3] = {0};
    byte stepMax[3] = {7,7,7};

  //gate parameter
    float globalDuty = 0.5;
    float duty[8] = {0.9, globalDuty, globalDuty, globalDuty, globalDuty, globalDuty, globalDuty, globalDuty}; //ratio between 0 and 1
    float gateOnSize[8]= {0}; //step duration for each step depending on duty value    
    unsigned int gateRand[8] = {0};
     

  //Control Voltage parameter
    byte initCV = 0;

    byte MIDINotes[3][8] = {{51, 52, 55, 56, 58, 59, 61, 63},

                            {53, 56, 59, 60, 55, 56, 59, 60},
                            
                            {56, 55, 52, 51, 56, 55, 52, 51}
                            };


  byte CV[3][8] = {0};
   /*  byte CV[3][8] = {{initCV, initCV, initCV, initCV, initCV, initCV, initCV, initCV},

                    {initCV, initCV, initCV, initCV, initCV, initCV, initCV, initCV},
                    
                    {initCV, initCV, initCV, initCV, initCV, initCV, initCV, initCV}
                    };
 */
    int tempCV10bit[8] = {0};
    byte octaveRange = 2; //octave range controlled by the potentiometers
    byte LowNoteMIDI = 45; //Low note set to 0 for the potentiometers
    byte HighNoteMIDI = LowNoteMIDI + octaveRange * 12;

    byte MIDImin = 33; //A1 is set to 0V
    byte MIDIMax = 93; //A6 is set to 5V we get 5 octave range.

  //Menus parameter
    bool menu = false;
    byte menuIndex = 1;
    byte track_M0 = 0;
    byte divIndex_M1 = 0;
    float division_M1[5] = {1, 4.0/3.0, 2, 3, 4};
    //stepMax_M2
    bool random_M3[] = {true, false, false};
    byte randomGate_M4 = 0;
    bool randomCV_M5[3] = {false, true, false};
    bool globalDutyBool_M6 = true;
    //globalDuty_M6
    byte clockSource_M7 = 0;
    bool clickMode_M8 = false;
    bool MIDIClockOut_M9 = true;
    //MIDI Mapping 

  //timers parameter
    int clockCount = 10000;
    int startTCNT2 = 255 - 25; //each interruption occurs when TCNT2 reach 255. TCNT2 is set to 230 so it counts 25 times between each interruption
    int varCompteur = 0;
    unsigned long gateOnFlag = 0;
    bool gateOffFlag = false;

    bool isGateOn = false;

  //MUX buttons
  bool resetFlag_True = false;
  bool resetFlag_False = false;

  bool leftFlag_True = false;
  bool leftFlag_False = false;

  bool rightFlag_True = false;
  bool rightFlag_False = false;

  bool dutyButtonBool = false;


/* 
███████╗███████╗████████╗██╗   ██╗██████╗ 
██╔════╝██╔════╝╚══██╔══╝██║   ██║██╔══██╗
███████╗█████╗     ██║   ██║   ██║██████╔╝
╚════██║██╔══╝     ██║   ██║   ██║██╔═══╝ 
███████║███████╗   ██║   ╚██████╔╝██║     
╚══════╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝      
*/
void setup() {
   //fill th CV matrix from MIDI values. CV[3][8] is set to {0} and has to be mapped to MIDI values

  for(int i = 0; i<3; i++){
    for(int j=0; j <8; j++){
      CV[i][j] = map(MIDINotes[i][j], MIDImin , MIDIMax, 0, 255);     
    }
  } 


  //counter init from locoduino.org
    cli(); // Global interruption off
    bitClear (TCCR2A, WGM20); // WGM20 = 0 bit 0 of TCCR2A
    bitClear (TCCR2A, WGM21); // WGM21 = 0 bit 1 of TCCR2A
    TCCR2B = 0b00000100; // Clock / 64 corresponds to 4 µs and WGM22 = 0 -> bit 3 from TCCR2B
    TIMSK2 = 0b00000000; // Local interruption by TOIE2 forbidden for the moment. The I/O button has to be clicked to turn it 
    
  //settings for the timer 4 responsible for the PWM outputs D6, D7 and D8
  // timer 4 is set by default to phase correct mode (precision of 510 steps), we want to increase PWM frequency in order to get a precise CV value.
  // PWM is converted to continuous current by a 2nd order Low Pass filter. the maximum frequency we can get from timer 4 (16bit) is 31372.5 Hz. 
  //To get this we have to set the prescaler for the 4th timer to 1 :    
    bitSet (TCCR4A, WGM40); // WGM20 = 0 bit 0 of TCCR2A
    bitClear (TCCR4A, WGM41); // WGM21 = 0 bit 1 of TCCR2A
    bitClear (TCCR4B, WGM42);
    TCCR4B &= 0b11111000; //clear the bits 2, 1 and 0 of TCCR4B register responsible for the prescaler value
    TCCR4B |= 0b00000001; //set the 3 first bits to 1 -> 001
 

  //init the pin four output and intput when necessary
    pinMode(neoPixelPin, OUTPUT);
    pinMode(ioPin,INPUT_PULLUP);
   // pinMode(resetPin, INPUT_PULLUP);

    buttonLeft.setDebounceTime(50);
    buttonRight.setDebounceTime(50);
    buttonRST.setDebounceTime(50);
    button1.setDebounceTime(50);

    pinMode(gateOut1, OUTPUT);
    pinMode(cvOut1, OUTPUT);
    pinMode(cvOut2, OUTPUT);
    pinMode(cvOut2, OUTPUT);

    pinMode(s0Mux, OUTPUT);
    pinMode(s1Mux, OUTPUT);
    pinMode(s2Mux, OUTPUT);
    pinMode(s3Mux, OUTPUT);

    digitalWrite(s0Mux, LOW);
    digitalWrite(s1Mux, LOW);
    digitalWrite(s2Mux, LOW);
    digitalWrite(s3Mux, LOW);
    

  //init de l'interruption i/O
    attachInterrupt(digitalPinToInterrupt(ioPin), ioButton, RISING);

  //
    pixels.begin(); // INITIALIZE NeoPixel strip object
    ledDisplay(16 - (stepCount[track_M0]+1)*2);

    setGateParam(bpm, duty);

   //Serial init
    Serial.begin(9600);
    Serial.println("Totem is set up.");
    Serial.print("Division mode :");
    Serial.println(division_M1[0]);
    Serial.print("Random steps : ");
    switch (random_M3[0]) {
    case 1 : 
    Serial.println("Shuffle");
    break;
    case 2 : 
    Serial.println("Random");
    break;    
    default:
    Serial.println("Linear");
    break;
    }    

    Serial.print("Click to confirm mode : ");
    clickMode_M8 ? Serial.println("On") : Serial.println("Off");
    
     Serial.print("Random CV track 1 :");
    randomCV_M5[0] ? Serial.println("On") : Serial.println("Off"); 

     Serial.print("Random CV track 2 :");
    randomCV_M5[1] ? Serial.println("On") : Serial.println("Off");  

      Serial.print("Random CV track 3 :");
    randomCV_M5[2] ? Serial.println("On") : Serial.println("Off");  
    
  //initialize random seed
    byte seed = getSeed();
    randomSeed(seed);    

  //Set MIDI baud rate:
  //Serial.begin(31250);

  //init du display
     if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
      Serial.println("SSD1306 allocation failed");
      for(;;); // Don't proceed, loop forever
    }
    display.clearDisplay();
    delay(2000);
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(30, 16);
    // Display static text
    display.println("ARCS");
    display.setTextSize(2);
    display.setCursor(16, 42);
    display.println("is ready!");
    display.display();
    delay(1000);
    oledTempo();
    
  //Setup ends with the possibility to start interruption
    sei(); // Allowing back global interruption . Interruptions should be allowed at the end if the setup() function to let the Arduino initialize correctly.
    TCNT2 = startTCNT2; //init timer at 230
    

}
/* 
██╗███╗   ██╗████████╗███████╗██████╗ ██████╗ ██╗   ██╗██████╗ ████████╗
██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗██╔══██╗██║   ██║██╔══██╗╚══██╔══╝
██║██╔██╗ ██║   ██║   █████╗  ██████╔╝██████╔╝██║   ██║██████╔╝   ██║   
██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗██╔══██╗██║   ██║██╔═══╝    ██║   
██║██║ ╚████║   ██║   ███████╗██║  ██║██║  ██║╚██████╔╝██║        ██║   
╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚═╝        ╚═╝    
*/

ISR(TIMER2_OVF_vect) {
  TCNT2 = startTCNT2; // 25 x 4 µS = 0.1 ms
  int midiCount = round(clockCount / 24);

  if(varCompteur % midiCount == 0){
   // MIDI.sendRealTime(midi::Clock);
  }

  if (varCompteur++ > clockCount) { // clockCount = durée d'un pas temporel en 0,1ms
    varCompteur = 0;
    isGateOn = true;
  }  
}


void ioButton(){
  ioState = !ioState;

  if(ioState){
   varCompteur = 0;
   isGateOn = true;
   //MIDI.sendRealTime(midi::Start);
   Serial.println("On");
   TIMSK2 = 0b00000001; // Local interrupt allowed for TOIE2
  }
  else{
   // MIDI.sendRealTime(midi::Stop);
    Serial.println("Off");
    TIMSK2 = 0b00000000; // Local interrupt forbidden for TOIE2
    gateOff(gateOut1);
    //pixels.clear();
    //pixels.show();

  } 
}

/* 
██╗      ██████╗  ██████╗ ██████╗ 
██║     ██╔═══██╗██╔═══██╗██╔══██╗
██║     ██║   ██║██║   ██║██████╔╝
██║     ██║   ██║██║   ██║██╔═══╝ 
███████╗╚██████╔╝╚██████╔╝██║     
╚══════╝ ╚═════╝  ╚═════╝ ╚═╝      
*/
//________________________________

void loop() {

  resetButton();
  dutyButton();  

 

  //open the gate at each interruption
  if(isGateOn){ 
    
    gateOn(gateOut1);
    for (int i = 0; i < 3; i++){    
      if (randomCV_M5[i] ==false){
      cvOut(i, CV[i] [stepCount[i]]);
      }
      else {
        byte randomCVtemp = 0;
        randomCVtemp = map(random(MIDImin, MIDIMax), MIDImin , MIDIMax, 0, 255);        
        cvOut(i, randomCVtemp);
      }
    }//for
    //MIDIOut();
    ledDisplay(16 - (stepCount[track_M0]+1)*2);
    isGateOn = false;
    gateOffFlag = true;

   for(int i = 0; i<= 2; i++){ 
      if(random_M3[i]){
        randomStep_NoRep(i);
      }
      else {
        linearStep(i);
      }
    }//for

    globalStepCount >= 7? globalStepCount = 0 : globalStepCount ++;


  }  //if(isGateOn)

  //wait the approrpiate amount of time before closing the gate
    unsigned long timerMillis = millis();
    byte stepTemp;
    
    globalStepCount == 0 ? stepTemp = 7 : stepTemp = globalStepCount - 1; 
      
    if(timerMillis > gateOnFlag + gateOnSize[stepTemp]){ 
      gateOff(gateOut1);
    }
      

// rotary switch KY040
    static int pos = 0;
    encoder.tick();

    int newPos = encoder.getPosition();
    if (pos != newPos) {
      if(menu ==false){
        newBpm += (float)encoder.getDirection();

        if(clickMode_M8){
        Serial.print("New Bpm = ");
        Serial.println(newBpm);
        }
        else {
        bpm = newBpm*division_M1[divIndex_M1];
        oledTempo();
        Serial.print("BPM = ");
        Serial.println(bpm);
        setGateParam(bpm, duty);
        }
      }
      else {
        menuEncoder();
      }
      pos = newPos;
    } // if

//read MUX inputs
  //Loop through and read the 8 first inputs (potentiometers)
  
  // int j = 0;
  for (int j = 0; j < 8; j ++){
    int actualValue = readMux(j);    
    if(abs(actualValue - tempCV10bit[j]) >=  round(1024 / (12 * octaveRange)) ){ //write the value if it changes a lot

      tempCV10bit[j] = actualValue;

      if(dutyButtonBool) {
        duty[j] = (float)map(tempCV10bit[j],0, 1023,0,100)/100;
        setGateParam(bpm, duty);
      }
      else {
        MIDINotes[track_M0][j] = map(actualValue, 0, 1023, LowNoteMIDI, HighNoteMIDI);  
        CV[track_M0][j] = map(MIDINotes[track_M0][j], MIDImin , MIDIMax, 0, 255);
      }
    }//if
  }//for 

  leftSelect();
  rightSelect();
  menuSelect();

} //LOOP


/* 
███████╗██╗   ██╗███╗   ██╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗███████╗
██╔════╝██║   ██║████╗  ██║██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║██╔════╝
█████╗  ██║   ██║██╔██╗ ██║██║        ██║   ██║██║   ██║██╔██╗ ██║███████╗
██╔══╝  ██║   ██║██║╚██╗██║██║        ██║   ██║██║   ██║██║╚██╗██║╚════██║
██║     ╚██████╔╝██║ ╚████║╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║███████║
╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝
 */
//________________________________________________________________________


void setGateParam(float bpm, float duty[]){

  clockCount = round((600000 / bpm))  ; // 0.1 ms

  for(int i=0; i<=7; i++){  //fixe une nouvelle valeur de durée de step en *10^-1ms (à chaque interruption Timer2_OVF_vect, clockCount = gateOnSize puis gateOffSize)
    
    gateOnSize[i] = (int)round((60000 / bpm) * duty[i]); //gate duration in ms

  } 
}
void gateOn(int ledPinOut){ 
  digitalWrite (ledPinOut, HIGH);
  gateOnFlag = millis();
}

void gateOff(int ledPinOut){
  digitalWrite (ledPinOut, LOW);
}

void cvOut(byte channel, byte CVvalue ){
  switch (channel) {
    case 0 : 
      analogWrite(cvOut1,CVvalue);
      break;
    case 1 : 
      analogWrite(cvOut2,CVvalue);
      break;
    case 2 : 
      analogWrite(cvOut3,CVvalue);
      break;
  }
}

//mux reading function from elektropeak.com
float readMux(int channel){
  int controlPin[] = {s0Mux, s1Mux, s2Mux, s3Mux};

  int muxChannel[16][4]={
    {0,0,0,0}, //channel 0
    {1,0,0,0}, //channel 1
    {0,1,0,0}, //channel 2
    {1,1,0,0}, //channel 3
    {0,0,1,0}, //channel 4
    {1,0,1,0}, //channel 5
    {0,1,1,0}, //channel 6
    {1,1,1,0}, //channel 7
    {0,0,0,1}, //channel 8
    {1,0,0,1}, //channel 9
    {0,1,0,1}, //channel 10
    {1,1,0,1}, //channel 11
    {0,0,1,1}, //channel 12
    {1,0,1,1}, //channel 13
    {0,1,1,1}, //channel 14
    {1,1,1,1}  //channel 15
  };

  //loop through the 4 sig
  for(int i = 0; i < 4; i ++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  //read the value at the SIG pin
  int val = analogRead(sigMux);

  //return the value
  //int voltage = round(val / 8); //we're interested in a value to control PWM pins. as it's red between 0 and 1023 we divide it by 4 to obtain a value btw 0 and 255
  return val;
}

//buttons related fucntions
void resetButton(){

  buttonRST.loop();

  if(buttonRST.isPressed()){
    
    for(int i=0; i < 3; i++) stepCount[i] = 0;
    
    if(!ioState){
      ledDisplay(16 - (stepCount[track_M0]+1)*2);
      for(int i = 0; i < 3; i++) cvOut(i, CV[i] [stepCount[i]]);
      }
    resetFlag_False = true;
  }
  
}

void leftSelect(){

  buttonLeft.loop();
  
  if(buttonLeft.isPressed()){
    if(menu == false){
      leftFlag_False = true;
      track_M0 <= 0 ? track_M0 = 2 : track_M0 --;
    }
    else{
      menuIndex<=0? menuIndex =10 : menuIndex -= 1;
      oledMenu();
    }
  }
  

}

void rightSelect(){

  buttonRight.loop();
  
  if(buttonRight.isPressed()){
    if(menu ==false){
      rightFlag_False = true;
      track_M0 >= 2 ? track_M0 = 0 : track_M0 ++;
    }
  
    else {
      menuIndex > 10? menuIndex = 1 : menuIndex += 1;
      oledMenu();
    }
  }

}
void menuSelect(){

   button1.loop();
  
  if(button1.isPressed()){
    menu = !menu;
    menu? oledMenu() : oledTempo();
    Serial.println(menu);
  }

}
void dutyButton(){
 
   readMux(11) > 500? dutyButtonBool = true : dutyButtonBool = false;

  if(dutyButtonBool){
    pixels.clear();
    for(int i = 0; i < (stepMax[0]+1)*2; i+=2) pixels.setPixelColor(14-i, pixels.Color(50, 50, 50));
    pixels.show(); 
  } 
   else if(dutyButtonBool ==false && ioState==false){
     ledDisplay(16 - (stepCount[track_M0]+1)*2);
  } 
}
void ledDisplay(int ledStep){
      pixels.clear();
      
      switch(track_M0){
        case 0 :
          for(int i = 0; i < (stepMax[0]+1)*2; i+=2) pixels.setPixelColor(14-i, pixels.Color(5, 0, 0));
          pixels.setPixelColor(ledStep, pixels.Color(100, 0, 0));

        break;
        case 1 :
          for(int i=0; i< (stepMax[1]+1)*2; i+=2) pixels.setPixelColor(14-i, pixels.Color(5, 0, 5));
          pixels.setPixelColor(ledStep, pixels.Color(100, 0, 100));
        break;
        case 2 : 
          for(int i=0; i< (stepMax[2]+1)*2; i+=2) pixels.setPixelColor(14-i, pixels.Color(0, 5, 0));
          pixels.setPixelColor(ledStep, pixels.Color(0, 75, 0));
        break;
      }
      
      pixels.show();
}

// functions used to generate random seed based on an analog entry :
  /* 
    to generate a random seed for the random() function, Arduino.cc advise to read a value on an unused Analog Pin of th ATMega. This leads to a great proportion of 0 and values near 254. To be shure the seed value is really a random one, we use the simple algorithm by Von Neumann in a 1951 article : "Various techniques used in connection with random digits" in Applied Math Series 12:36-38. By reading an analog Pin, we set bit pairs. If their're similar their're ignored. If different, we keep the first bit to be part of the seed. This technique leads to a more even distribution of each probability.
  */
byte getSeed(){ //called in the Setup() from LOCODUINO.org
      byte seed = 0;
      byte i = 8;
      while (i > 0) {
        byte firstBit = analogRead(seedPin) & 0x01;
        byte secondBit = analogRead(seedPin) & 0x01;
        if (firstBit != secondBit) {
          seed = (seed << 1) | firstBit;
          i--;
        }
      }
      return seed;
}

void linearStep(int i){
        if (stepCount[i] >= stepMax[i]){
          stepCount[i] = 0;    
        }
        else { 
          stepCount[i] ++;
        }      
}

void randomStep_NoRep(int i){

          oldStep[i] = stepCount[i];
          byte temp = oldStep[i];
          while(temp == oldStep[i]){
            temp = random(0,stepMax[i]+1);
          }
          stepCount[i] = temp;
}

void oledTempo(){

    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(50, 16);
    // Display static text
    display.println(bpm);
    display.display();
}

void oledMenu(){


  switch(menuIndex){
  
    case 1:
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(10, 16);
      // Display static text
      display.println("Division mode : ");
      display.print(division_M1[divIndex_M1]);
      display.display(); 
      break;
    
    default: 
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(10, 16);
      // Display static text
      display.println("Parameters (use < & > to navigate) ");
      display.display();  
      break;
    }
}

void menuEncoder(){

  switch(menuIndex){
    case 1 : 
      
      byte incr = (float)encoder.getDirection();
      float oldDiv = division_M1[divIndex_M1];

      divIndex_M1 += incr;
      if(divIndex_M1 == 255) divIndex_M1 = 0;
      else if (divIndex_M1 >= 5) divIndex_M1 = 4;
     
      //bpm = (bpm/oldDiv) * division_M1[divIndex_M1];
      setGateParam(bpm* division_M1[divIndex_M1], duty);
      oledMenu();
    break;

    default:

    break;

  }
}

// MIDI communication protocols
/* 
void MIDIOut(){
 MIDI.send(midi::NoteOn, MIDINotes[0][stepCount[0]], 127, 1);
 MIDI.send(midi::NoteOn, MIDINotes[1][stepCount[1]], 127, 2);
 MIDI.send(midi::NoteOn, MIDINotes[2][stepCount[2]], 127, 3);
}
 */
