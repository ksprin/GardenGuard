#include "LoRaRadio.h"
#include "TimerMillis.h"

// maybe use clock interrupt to send last data to computer (serial output)
// maybe use clock interrupt to listen to computer

TimerMillis writeToComputerClock;
TimerMillis readFromComputerClock;

int pirEvents = 0;
int temperature = 0;
int moistureLevel = 0;
int buzzerState = 0; // init on 0: buzzer off
int buzzerOverride = 0;
int dry = 0;
int wet = 100;

char dataComputer = 'x';

// Char codes
// x - null
// r - read from computer serial port
// n - no read

/*
void writeToComputerClockHandler(void){
  //Serial.print("Writing to computer: ");
  //Serial.println(dataComputer);
  Serial.println(", "+String(moistureLevel)+", "+String(temperature)+", "+String(pirEvents)+", "+String(buzzerCondition));

}
*/



void readFromComputerClockHandler(void){
  // send string to indicate receiving
  Serial.println("r");
  delay(2000);
  if (Serial.available() > 0){
    dataComputer = Serial.read();
    Serial.println(dataComputer); //would not actually print in finished system
  }
  else{
    Serial.println("n");
  }
}


static void receiveCallback(void){
  if ((LoRaRadio.parsePacket() == 9) &&
      (LoRaRadio.read() == 'S') &&
      (LoRaRadio.read() == 'P'))
  {
    pirEvents = LoRaRadio.read();
    moistureLevel = LoRaRadio.read();
    temperature = LoRaRadio.read();
    buzzerState = LoRaRadio.read();
    buzzerOverride = LoRaRadio.read();
    wet = LoRaRadio.read();
    dry = LoRaRadio.read();
    //Serial.println("Received signal: ");
    //Serial.print("Detections: ");
    //Serial.println(pirEvents);
    //Serial.print("Moisture Data: ");
    //Serial.println(moistureLevel);
    // got signal so send confirm

    Serial.println(","+String(moistureLevel)+","+String(temperature)+","+String(pirEvents)+","+String(buzzerState)+","+String(buzzerOverride)+","+String(dry)+","+String(wet));
    //if (!LoRaRadio.busy()){
      LoRaRadio.beginPacket();
      LoRaRadio.write('O');
      LoRaRadio.write('K');
      LoRaRadio.write(dataComputer);
      LoRaRadio.endPacket();
      //Serial.println("sent OK");
    //}
    
    //LoRaRadio.receive();
  }
  else{
    //if (!LoRaRadio.busy()){
      LoRaRadio.beginPacket();
      LoRaRadio.write('N');
      LoRaRadio.write('O');
      LoRaRadio.write('x');
      LoRaRadio.endPacket();
      //Serial.println("sent NO");
    //}
    //LoRaRadio.receive();
  //Serial.println("sent2");
  }
}

static void transmitCallback(void){
  LoRaRadio.receive();
}

void setup() {
  Serial.begin(9600);
    
  while (!Serial) { }

  LoRaRadio.begin(915000000);

  LoRaRadio.setFrequency(915000000);
  LoRaRadio.setTxPower(14);
  LoRaRadio.setBandwidth(LoRaRadio.BW_125);
  LoRaRadio.setSpreadingFactor(LoRaRadio.SF_7);
  LoRaRadio.setCodingRate(LoRaRadio.CR_4_5);
  LoRaRadio.setLnaBoost(true);

  LoRaRadio.onTransmit(transmitCallback);
  LoRaRadio.onReceive(receiveCallback);

  //writeToComputerClock.start(writeToComputerClockHandler, 5000, 5000);
  readFromComputerClock.start(readFromComputerClockHandler, 15000, 30000);

  LoRaRadio.receive(500);
}

void loop() {
  //LoRaRadio.receive(1000);

  // packet to send 
  // first unique identifier: string
  // array of data
}
