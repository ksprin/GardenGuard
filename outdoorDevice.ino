#include "LoRaRadio.h"
#include "TimerMillis.h"
#include "STM32L0.h"

TimerMillis clock;
TimerMillis buzzerClock;

bool readyToPrint = false;
bool sending = false;
bool received = false;
int detected;
int moistureLevel;
int temperature;
int buzzerState; 
int buzzerOverride;
int calibrationState; 
int dry;
int wet;
int sleepTime;
int pirStatus;
int pirFlag;
char temp1;
char temp2;
int overrideSwitch;

int releaseTime;
int pressTime;

int calibrateFlag;

#define STATE_INIT        0
#define STATE_DRY    1
#define STATE_WET   2
#define STATE_ENDING  3

char dataComputer = 'x'; // x is default char

static void calibrate( void ){
  calibrationState = STATE_INIT;
  digitalWrite(PIN_LED2, HIGH);
  digitalWrite(12, HIGH);

  // hook up seperate button interupts
  attachInterrupt(6, calibrateButton, CHANGE);

  while(calibrationState < STATE_ENDING){
    if (calibrationState == STATE_DRY){
      dry = analogRead(16) / 10;
      
    }
    if (calibrationState == STATE_WET){
      wet = analogRead(16) / 10;
      
    }
  }
  // ending
  digitalWrite(PIN_LED2, LOW);
  digitalWrite(12, LOW);
  detachInterrupt(6);

}

void calibrateButton( void ){
  int status = digitalRead(6);

  if (status == HIGH){
    // pressed
    pressTime = millis();
  }
  else{
    // open
    releaseTime = millis();

    //calc press time: accept if over 0.25 second
    if ((releaseTime - pressTime) >= 60){
      // button press: change state
      calibrationState = calibrationState + 1;
    }
  }
}

void userButton( void ){
  int status = digitalRead(6);
  
  if (status == LOW){
    // pressed
    pressTime = millis();
  }
  else{
    // open
    releaseTime = millis();

    int diff =  releaseTime - pressTime;
    //calc press time: accept if over 0.25 second
    if (diff >= 60){
      // long press
      calibrateFlag = !calibrateFlag;
    }
    STM32L0.wakeup();
  }
}

// Sending function
static void send ( void ) {
  if (sending){
    if (!LoRaRadio.busy()){

      LoRaRadio.beginPacket();
      LoRaRadio.write('S');
      LoRaRadio.write('P');
      LoRaRadio.write(detected);
      LoRaRadio.write(moistureLevel);
      LoRaRadio.write(temp1);
      LoRaRadio.write(temp2);
      LoRaRadio.write(buzzerState);
      LoRaRadio.write(buzzerOverride);
      LoRaRadio.write(wet);
      LoRaRadio.write(dry);
      LoRaRadio.endPacket();
 
      sending = false;
    }
  }
  
  if (LoRaRadio.parsePacket() == 3){
     if ((LoRaRadio.read() == 'O') &&
      (LoRaRadio.read() == 'K')){
        // revceived confirmation
        received = true;
        dataComputer = LoRaRadio.read();
        // i : buzzer on
        // o : buzzer off
        if (dataComputer == 'i'){
          buzzerState = 1;
        }
        else if (dataComputer == 'o'){
          buzzerState = 0;
        }
      }
  }
  

}

static void transmitCallback ( void ){
  LoRaRadio.receive(250);
}


void pirInterupt(void){
  detected = detected + 1;

  overrideSwitch = digitalRead(9);
  if (overrideSwitch){
    buzzerOverride = 1;
  }
  else{
    buzzerOverride = 0;
  }

  if (!buzzerOverride && buzzerState){

      digitalWrite(8, HIGH);
      delay(1000);
      digitalWrite(8, LOW);

  }

}

void setup( void ) {
  
    Serial.begin(9600);
    
    while (!Serial) { }
  
    LoRaRadio.begin(915000000);

    LoRaRadio.setFrequency(915000000);
    LoRaRadio.setTxPower(14);
    LoRaRadio.setBandwidth(LoRaRadio.BW_125);
    LoRaRadio.setSpreadingFactor(LoRaRadio.SF_12);
    LoRaRadio.setCodingRate(LoRaRadio.CR_4_5);
    LoRaRadio.setLnaBoost(true);

    detected = 0;
    moistureLevel = 0;
    buzzerState = 1;
    buzzerOverride = 0;
    temperature = 0;
    calibrationState = 0;
    dry = 0;
    wet = 100;
    releaseTime = 0;
    pressTime = 0;
    calibrateFlag = 0;
    sleepTime = 12000;
    pirStatus = 0;
    pirFlag = 0;
    overrideSwitch = 0;

    // set output on PA_8: GPIO out (7) : thermistor power
    pinMode(7, OUTPUT); 

    // Thermistor A2 (18)
    pinMode(18, INPUT);

    // set output on PA_9: GPIO out (8) : buzzer in
    // buzzer power is 3.3V
    pinMode(8, OUTPUT);

    // set interupt on PB_15: GPIO in (11) : PIR output
    pinMode(11, INPUT); 

    // Moisture A0 (16)
    pinMode(16, INPUT); 

    // Moisture Sensor Power: PB_14: GPIO out (12)
    pinMode(12, OUTPUT);

    // Switch power: PB_13: GPIO out (13)
    pinMode(13, OUTPUT);

    // Switch in: PB_12: GPIO in (9)
    // Buzzer is override (off) with on (switch high)
    pinMode(9, INPUT);

    // User Button
    pinMode(6, INPUT);

    // LED
    pinMode(PIN_LED2, OUTPUT);

    LoRaRadio.onTransmit(transmitCallback);
    LoRaRadio.onReceive(send);
    attachInterrupt(11, pirInterupt, FALLING);
    attachInterrupt(6, userButton, CHANGE);
    digitalWrite(PIN_LED2, LOW);
    digitalWrite(13, HIGH);

}

void loop() {
  detachInterrupt(11);
  detachInterrupt(6);

  if (calibrateFlag){
    // call calibrate method
    calibrate();
    calibrateFlag = 0;
  }

  overrideSwitch = digitalRead(9);
  if (overrideSwitch){
    buzzerOverride = 1;
  }
  else{
    buzzerOverride = 0;
  }
  
  digitalWrite(12, HIGH);
  digitalWrite(7, HIGH);
  delay(250);
  moistureLevel = analogRead(16) / 10;
  temperature = analogRead(18);
  digitalWrite(12, LOW);
  digitalWrite(7, LOW);

  temp1 = (char) (temperature>>8);
  temp2 = (char) (temperature);

  sending = true;
  received = false;
  LoRaRadio.receive(1);
  
  for(int i = 0; i < 500; i++){
    delay(1);
  } 

  if (!received){
    sending = true;
    LoRaRadio.receive(1);
    for(int i = 0; i < 500; i++){
      delay(1);
    }
  }

  detected = 0; 
  sending = false;
  
  attachInterrupt(11, pirInterupt, FALLING);
  attachInterrupt(6, userButton, CHANGE);
  STM32L0.sleep(sleepTime);
  
}
