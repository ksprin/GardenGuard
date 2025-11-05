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

int releaseTime;
int pressTime;

int calibrateFlag;
int buzzerFlag;

long nextSchedule;
long lastSchedule;

int remainingSleep;

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
      LoRaRadio.write(temp1);
      LoRaRadio.write(temp2);
      LoRaRadio.write(buzzerState);
      LoRaRadio.write(buzzerOverride);
      LoRaRadio.write(wet);
      LoRaRadio.write(dry);
      LoRaRadio.endPacket();
      Serial.println("sent");
      sending = false;
    }
  }
  delay(10);
  
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
  pirFlag = 1;
  // if buzzerOverride is deactivated: use computer value

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
    buzzerState = 1; // init as Off
    buzzerOverride = 0;
    temperature = 0;
    calibrationState = 0;
    dry = 0;
    wet = 100;
    releaseTime = 0;
    pressTime = 0;
    calibrateFlag = 0;
    buzzerFlag = 0;
    sleepTime = 20000;
    pirStatus = 0;
    pirFlag = 0;

    nextSchedule = 0;
    lastSchedule = 0;
    remainingSleep = 20000;

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

    // User Button
    pinMode(6, INPUT);

    // LED
    pinMode(PIN_LED2, OUTPUT);

    LoRaRadio.onTransmit(transmitCallback);
    LoRaRadio.onReceive(send);
    attachInterrupt(11, pirInterupt, FALLING);
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

  if (pirFlag){
    if (!buzzerOverride && buzzerState){
      Serial.println("BUZZ");

      digitalWrite(8, HIGH);
      delay(1000);
      digitalWrite(8, LOW);

    }
    // calc diff for sleep time
    remainingSleep = sleepTime - (millis() - lastSchedule);
    Serial.println(remainingSleep);
    if (remainingSleep > sleepTime || remainingSleep < 0){
      remainingSleep = sleepTime;
    }
    pirFlag = 0;
  }
  else{
    remainingSleep = sleepTime;
    lastSchedule = millis();
    nextSchedule = lastSchedule + sleepTime;
  }
  Serial.println(remainingSleep);
  
  // can comment when no sensors attatched
  digitalWrite(12, HIGH);
  digitalWrite(7, HIGH);
  delay(250);
  moistureLevel = analogRead(16) / 10;
  temperature = analogRead(18);
  digitalWrite(12, LOW);
  digitalWrite(7, LOW);

  temp1 = (char) (temperature>>8);
  temp2 = (char) (temperature);

  Serial.print("Moisture Data: ");
  Serial.println(moistureLevel);
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Dry: ");
  Serial.println(dry);
  Serial.print("Wet: ");
  Serial.println(wet);
  Serial.print("Buzzer Status: ");
  Serial.println(buzzerState);
  Serial.print("Buzzer Override: ");
  Serial.println(buzzerOverride);

  sending = true;
  received = false;
  LoRaRadio.receive(10);
  
  //sending = false;
  delay(2000); //delay to send can probably make less time 

  if (!received){
    sending = true;
    LoRaRadio.receive(10);
    delay(1200);
  }

  detected = 0; //reset
  sending = false;
  
  attachInterrupt(11, pirInterupt, FALLING);
  attachInterrupt(6, userButton, CHANGE);
  STM32L0.sleep(remainingSleep); // calls LoRaRadio.receive for some reason
  
}
