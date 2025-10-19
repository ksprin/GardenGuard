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
int buzzerCondition; 

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


// Sending function
static void send ( void ) {
  if (sending){
    if (!LoRaRadio.busy()){

      LoRaRadio.beginPacket();
      LoRaRadio.write('S');
      LoRaRadio.write('E');
      LoRaRadio.write('N');
      LoRaRadio.write('D');
      LoRaRadio.write(detected);
      LoRaRadio.write(moistureLevel);
      LoRaRadio.write(temperature);
      LoRaRadio.write(buzzerCondition);
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
  //Serial.println("detected");
  //readyToPrint = true;
  detected = detected + 1;
  
  //prepSend();
  // Send LoRa transmission in 10ms
  //LoRaRadio.receive(10);

  
  // turn power for buzzer on
  //digitalWrite(7, HIGH);
  //delay(250);

  //digitalWrite(8, HIGH);
  //delay(1000);
  //digitalWrite(8, LOW);

  //digitalWrite(7, LOW);
  //LoRaRadio.receive(10);
  
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
    buzzerCondition = 1;
    temperature = 0;
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

    LoRaRadio.onTransmit(transmitCallback);
    LoRaRadio.onReceive(send);
    attachInterrupt(11, pirInterupt, RISING);

    //clock.start(timerInterupt, 0, 30000);

}

void loop() {
  detachInterrupt(11);
  Serial.print("detected: ");
  Serial.println(detected);

  digitalWrite(12, HIGH);
  delay(250);
  moistureLevel = analogRead(16);
  digitalWrite(12, LOW);
  Serial.print("Moisture Data: ");
  Serial.println(moistureLevel);
  sending = true;
  LoRaRadio.receive(10);
  
  //sending = false;
  delay(1200); //delay to send can probably make less time 
  detected = 0; //reset
  sending = false;
  attachInterrupt(11, pirInterupt, RISING);
  STM32L0.sleep(20000); // calls LoRaRadio.receive for some reason
  
}
