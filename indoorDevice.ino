#include "LoRaRadio.h"

static void receiveCallback(void){
  if ((LoRaRadio.parsePacket() == 6) &&
      (LoRaRadio.read() == 'S') &&
      (LoRaRadio.read() == 'E') &&
      (LoRaRadio.read() == 'N') &&
      (LoRaRadio.read() == 'D'))
  {
    int pirEvents = LoRaRadio.read();
    int moistureLevel = LoRaRadio.read();
    Serial.println("Received signal: ");
    Serial.print("Detections: ");
    Serial.println(pirEvents);
    Serial.print("Moisture Data: ");
    Serial.println(moistureLevel);
    // got signal so send confirm
    
    //if (!LoRaRadio.busy()){
      LoRaRadio.beginPacket();
      LoRaRadio.write('O');
      LoRaRadio.write('K');
      LoRaRadio.endPacket();
      Serial.println("sent OK");
    //}
    
    //LoRaRadio.receive();
  }
  else{
    //if (!LoRaRadio.busy()){
      LoRaRadio.beginPacket();
      LoRaRadio.write('N');
      LoRaRadio.write('O');
      LoRaRadio.endPacket();
      Serial.println("sent NO");
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
  LoRaRadio.receive(500);
}

void loop() {
  //LoRaRadio.receive(1000);

  // packet to send 
  // first unique identifier: string
  // array of data
}
