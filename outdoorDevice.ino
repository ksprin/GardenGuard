#include "LoRaRadio.h"
#include "TimerMillis.h"
#include "STM32L0.h"

TimerMillis clock;
TimerMillis buzzerClock;

bool readyToPrint = false;
int detected;
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
  if (!LoRaRadio.busy()){

    LoRaRadio.beginPacket();
    LoRaRadio.write('S');
    LoRaRadio.write('E');
    LoRaRadio.write('N');
    LoRaRadio.write('D');
    LoRaRadio.write(detected);
    LoRaRadio.endPacket();
    Serial.println("sent");
  }

}

/*
static void transmitCallback ( void ) {
  // wait to get confirmation
  //delay(1000);
  if (LoRaRadio.parsePacket() == 2){
     if ((LoRaRadio.read() == 'O') &&
      (LoRaRadio.read() == 'K')){
        // revceived confirmation
        Serial.println("Indoor device received message");
      }
  }
  //else{
  //  LoRaRadio.receive(10);
  //}
}
*/

/*
void timerInterupt(void){
  Serial.println("timer");
  LoRaRadio.receive(10); // Send LoRa transmission in 10ms
}
*/

/*
void prepSend(void){
    LoRaRadio.receive(1);
    delay(500); //delay to send
}
*/

void pirInterupt(void){
  //STM32L0.wakeup();
  Serial.println("detected");
  //readyToPrint = true;
  detected = detected + 1;
  //prepSend();
  // Send LoRa transmission in 10ms
  //LoRaRadio.receive(10);

  /*
  // turn power for buzzer on
  digitalWrite(7, HIGH);
  delay(250);
  digitalWrite(8, HIGH);
  delay(1000);
  digitalWrite(8, LOW);
  digitalWrite(7, LOW);
  //LoRaRadio.receive(10);
  */
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
    // put power on PB_14: GPIO out (12)
    //pinMode(12, OUTPUT);

    // set output on PA_8: GPIO out (7) : buzzer power
    //pinMode(7, OUTPUT);

    // set output on PA_9: GPIO out (8) : buzzer in
    //pinMode(8, OUTPUT);

    // set interupt on PB_15: GPIO in (11) : PIR output
    pinMode(11, INPUT); 

    //LoRaRadio.onTransmit(transmitCallback);
    LoRaRadio.onReceive(send);
    attachInterrupt(11, pirInterupt, RISING);
    //clock.start(timerInterupt, 0, 30000);

}

void loop() {
  //LoRaRadio.receive(10); // Send LoRa transmission in 10ms
  //delay(500); //delay to send
  //STM32L0.sleep(10000); // might want to expiriment with deep sleep
  
   //use to only send on interupt flag
  /*
  if (readyToPrint){
    Serial.println("detected print");
    readyToPrint = false;
    // Send LoRa transmission in 10ms
    //LoRaRadio.receive(10);
    //timerInterupt();
    //delay(500); //delay to send
  }
  else{
    Serial.println("print");
  }
  //unhook PIT interupt for sending
  */
  detachInterrupt(11);
  Serial.print("detected: ");
  Serial.println(detected);
  LoRaRadio.receive(10);
  delay(500); //delay to send
  detected = 0; //reset
  attachInterrupt(11, pirInterupt, RISING);
  STM32L0.sleep(20000);
  
}
