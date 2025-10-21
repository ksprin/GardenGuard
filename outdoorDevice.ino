#include "LoRaRadio.h"
#include "TimerMillis.h"
#include "STM32L0.h"

TimerMillis clock;
TimerMillis buzzerClock;

bool readyToPrint = false;
bool sending = false;
int detected;
int moistureLevel;
int temperature;
int buzzerState; 
int buzzerOverride;
int calibrationState; 
int dry;
int wet;

int releaseTime;
int pressTime;

int calibrateFlag;
int buzzerFlag;

#define STATE_INIT        0
#define STATE_DRY    1
#define STATE_WET   2
#define STATE_ENDING  3

char dataComputer = 'x'; // x is default char
// Send information on Interupt
// Only activating send (with LoRaRadio.receive)

/*
    void  enablePowerSave();
    void  disablePowerSave();
    void  wakeup();
    void  sleep(uint32_t timeout = 0xffffffff);
    void  deepsleep(uint32_t timeout = 0xffffffff);
    void  standby(uint32_t timeout = 0xffffffff);
    void  reset();
    void  dfu();
*/

static void calibrate( void ){
  calibrationState = STATE_INIT;
  digitalWrite(PIN_LED2, HIGH);
  digitalWrite(12, HIGH);

  // hook up seperate button interupts
  attachInterrupt(6, calibrateButton, CHANGE); // can only have one interupt so will need to check

  while(calibrationState < STATE_ENDING){
    if (calibrationState == STATE_DRY){
      dry = analogRead(16) / 10;
      
    }
    if (calibrationState == STATE_WET){
      wet = analogRead(16) / 10;
      
    }
  }
  // ending
  Serial.println("ending");
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
    if (diff >= 60 && diff < 2000){
      // short press
      buzzerFlag = !buzzerFlag;
    }
    else if (diff >= 2000){
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
      LoRaRadio.write(temperature);
      LoRaRadio.write(buzzerState);
      LoRaRadio.write(buzzerOverride);
      LoRaRadio.write(wet);
      LoRaRadio.write(dry);
      LoRaRadio.endPacket();
      Serial.println("sent");
      sending = false;
    }
  }
  
  if (LoRaRadio.parsePacket() == 3){
     if ((LoRaRadio.read() == 'O') &&
      (LoRaRadio.read() == 'K')){
        // revceived confirmation
        dataComputer = LoRaRadio.read();
        // i : buzzer on
        // o : buzzer off
        if (dataComputer == 'i'){
          buzzerState = 1;
        }
        else if (dataComputer == 'o'){
          buzzerState = 0;
        }
        Serial.println("Indoor device received last message");
        Serial.println(dataComputer);
      }
  }
  

}

static void transmitCallback ( void ){
  LoRaRadio.receive(1000);
}

/*
void timerInterupt(void){
  Serial.println("timer");
  LoRaRadio.receive(10); // Send LoRa transmission in 10ms
}
*/


void pirInterupt(void){
  STM32L0.wakeup();
  detected = detected + 1;

  if (!buzzerOverride && buzzerState){
    Serial.println("BUZZ");
    // turn power for buzzer on
    //digitalWrite(7, HIGH);
    //delay(250);

    //digitalWrite(8, HIGH);
    //delay(1000);
    //digitalWrite(8, LOW);

    //digitalWrite(7, LOW);
  }
  
}

void setup( void ) {
  
    Serial.begin(9600);
    
    while (!Serial) { }
  
    LoRaRadio.begin(915000000);

    LoRaRadio.setFrequency(915000000);
    LoRaRadio.setTxPower(14);
    LoRaRadio.setBandwidth(LoRaRadio.BW_125);
    LoRaRadio.setSpreadingFactor(LoRaRadio.SF_7);
    LoRaRadio.setCodingRate(LoRaRadio.CR_4_5);
    LoRaRadio.setLnaBoost(true);

    detected = 0;
    moistureLevel = 0;
    buzzerState = 0; // init as Off
    buzzerOverride = 0;
    temperature = 0;
    calibrationState = 0;
    dry = 0;
    wet = 100;
    releaseTime = 0;
    pressTime = 0;
    calibrateFlag = 0;
    buzzerFlag = 0;
    // put power on PB_14: GPIO out (12)
    //pinMode(12, OUTPUT);

    // set output on PA_8: GPIO out (7) : buzzer power
    //pinMode(7, OUTPUT);

    // set output on PA_9: GPIO out (8) : buzzer in
    //pinMode(8, OUTPUT);

    // set interupt on PB_15: GPIO in (11) : PIR output
    pinMode(11, INPUT); 

    // Moisture A0 (16)
    pinMode(16, INPUT); 

    // Moisture Sensor Power: PB_14: GPIO out (12)
    pinMode(12, OUTPUT);

    // User Button
    pinMode(6, INPUT);

    // LED
    pinMode(PIN_LED2, OUTPUT);

    LoRaRadio.onTransmit(transmitCallback);
    LoRaRadio.onReceive(send);
    attachInterrupt(11, pirInterupt, RISING);
    attachInterrupt(6, userButton, CHANGE);
    digitalWrite(PIN_LED2, LOW);
    //clock.start(timerInterupt, 0, 30000);

}

void loop() {
  detachInterrupt(11);
  detachInterrupt(6);
  Serial.print("detected: ");
  Serial.println(detected);

  if (calibrateFlag){
    // call calibrate method
    Serial.println("calibrate");
    calibrate();
    calibrateFlag = 0;
  }

  if (buzzerFlag){
    Serial.println("buzzer override");
    buzzerOverride = !buzzerOverride;
    buzzerFlag = 0;
  }

  digitalWrite(12, HIGH);
  delay(250);
  moistureLevel = analogRead(16) / 10;
  digitalWrite(12, LOW);

  Serial.print("Moisture Data: ");
  Serial.println(moistureLevel);
  Serial.print("Dry: ");
  Serial.println(dry);
  Serial.print("Wet: ");
  Serial.println(wet);
  Serial.print("Buzzer Status: ");
  Serial.println(buzzerState);
  Serial.print("Buzzer Override: ");
  Serial.println(buzzerOverride);

  sending = true;
  LoRaRadio.receive(10);
  
  //sending = false;
  delay(1200); //delay to send can probably make less time 
  detected = 0; //reset
  sending = false;
  attachInterrupt(11, pirInterupt, RISING);
  attachInterrupt(6, userButton, CHANGE);
  STM32L0.sleep(20000); // calls LoRaRadio.receive for some reason
  
}
