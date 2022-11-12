#include <TimerOne.h>
#include <TM1637.h>

/*
 * GENERAL PARAMETERS
 */

long intervalMicroSeconds;
float bpm = 126;  
byte counter = 0; 
byte pulse_per_beat = 1;
byte reset_on_beat = 16;

#define TM1637_DISPLAY
#define TM1637_CLK_PIN 10
#define TM1637_DIO_PIN 11
#define TM1637_BRIGHTNESS 0x0f

TM1637 tm(TM1637_CLK_PIN, TM1637_DIO_PIN);

// Rotary Encoder Inputs
#define inputCLK 9
#define inputDT 8
#define inputSW 7

byte menuState = 0;

byte currentStateSW;
byte previousStateSW;

byte currentStateCLK;
byte previousStateCLK; 

// 595 output pins
byte outputLatchPin = 5; 
byte outputClockPin = 6; 
byte outputDataPin = 4;  

//Status
byte statusClock = 0; // 0 = stopped, 1 = playing, 2 = pause

#define startTrigger 3
#define stopTrigger 2

bool TriggerPlay = false;
bool TriggerStop = false;

long debouncing_time = 150; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;

volatile long displayTimer;
int displayTimeOut = 500;

void setup() {

  Timer1.initialize(intervalMicroSeconds);
  Timer1.setPeriod(calculateIntervalMicroSecs(bpm));
  Timer1.attachInterrupt(sendClockPulse);

  //pinMode (stopTrigger,INPUT);
  //pinMode (startTrigger,INPUT);
    
  attachInterrupt(digitalPinToInterrupt(stopTrigger), stopClock, FALLING );
  attachInterrupt(digitalPinToInterrupt(startTrigger), pauseClock, FALLING );

  tm.begin();
  tm.setBrightness(TM1637_BRIGHTNESS);
  setDisplayValue(bpm);

  pinMode (inputCLK,INPUT_PULLUP);
  pinMode (inputDT,INPUT_PULLUP);
  pinMode (inputSW,INPUT);

  // Set all the pins of 74HC595 as OUTPUT
  pinMode(outputLatchPin, OUTPUT);
  pinMode(outputDataPin, OUTPUT);  
  pinMode(outputClockPin, OUTPUT);

  previousStateCLK = digitalRead(inputCLK);
  previousStateSW = digitalRead(inputSW);
  
  //Serial.begin (9600);
}

void loop() {
   
   currentStateSW = digitalRead(inputSW);
   
   if (currentStateSW != previousStateSW && currentStateSW == 0) {
      if(menuState >= 3) {
        menuState = 0;
      } else {
        menuState++;
      }
   }
   previousStateSW = currentStateSW; 

   if (displayTimer < millis()) {
     if (menuState == 0 || menuState == 1 ) {
      setDisplayValueFloat(bpm);
     }
     if (menuState == 2) {
      setDisplayValue(pulse_per_beat);
     }
     if (menuState == 3) {
      setDisplayValue(reset_on_beat);
     }
   }
  
   currentStateCLK = digitalRead(inputCLK);

   if (currentStateCLK != previousStateCLK && currentStateCLK == 0){   
     if (menuState == 0) {     
       if (digitalRead(inputDT) != currentStateCLK) { 
         bpm++; 
       } else {
         bpm--;
       }
     }
     
     if (menuState == 1) {     
       if (digitalRead(inputDT) != currentStateCLK) { 
         bpm += +0.1; 
       } else {
         bpm += -0.1;
       }
     }
     
     if (menuState == 2) {     
       if (digitalRead(inputDT) != currentStateCLK) { 
         pulse_per_beat++; 
       } else {
         pulse_per_beat--;
       }
     }

     if (menuState == 3) {     
       if (digitalRead(inputDT) != currentStateCLK) { 
         reset_on_beat++; 
       } else {
         reset_on_beat--;
       }
     }
     updateBpm(calculateIntervalMicroSecs(bpm));
   }   
   previousStateCLK = currentStateCLK; 

   if(TriggerPlay) {
    if(statusClock == 1) {
      statusClock = 2;
      tm.display("PAUS");
    } else {
      statusClock = 1;
      tm.display("PLAY");
    }
    displayTimer = millis() + displayTimeOut;
    last_micros = micros();
    TriggerPlay = false;
   }

   if(TriggerStop) {
    tm.display("STOP");
    displayTimer = millis() + displayTimeOut;
    last_micros = micros();
    counter = 0;
    statusClock = 0;
  
    outputClock(B11110000);
    delay(4);
    outputClock(B00000000);
    TriggerStop = false;
   }
}

void updateBpm(long now) {
  long interval = calculateIntervalMicroSecs(bpm);
  Timer1.setPeriod(interval);
}

void sendClockPulse() {
  if (statusClock == 1) {
    counter++;
    
    outputClock(B00001111);
    delay(4);
    outputClock(B00000000);

    if(counter >= (reset_on_beat*pulse_per_beat) ) {
      counter = 0;
      outputClock(B11110000);
      delay(4);
      outputClock(B00000000);
    } 
  }
}

void pauseClock() {
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    TriggerPlay = true;
  }
}


void stopClock() {
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    TriggerStop = true;
  }
}

void outputClock(byte pins)
{
   digitalWrite(outputLatchPin, LOW);
   shiftOut(outputDataPin, outputClockPin, LSBFIRST, pins);
   digitalWrite(outputLatchPin, HIGH);
}

long calculateIntervalMicroSecs(int bpm) {
  return 60L * 1000 * 1000 / bpm / pulse_per_beat;
}

void setDisplayValue(int value) {
  byte empty = 0;
  if(value < 10) {
    empty++;
  }
  if(value < 100) {
    empty++;
  }
  if(value < 1000) {
    empty++;
  }
  tm.display(value, false, false,empty);
}

void setDisplayValueFloat(float value) {
  if(statusClock == 1) {
    tm.display(value)->scrollLeft(displayTimeOut);
  } else if(statusClock == 0) { 
    // Somewhat buggie code here but does the job.
    // Need better fix next commit.    
    tm.setBrightness(TM1637_BRIGHTNESS);
    tm.display(value)->blink(displayTimeOut);
  } else {
    tm.display(value);
  }
}
