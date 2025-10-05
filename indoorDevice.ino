#include "LoRaRadio.h"

static void receiveCallback(void){
  if ((LoRaRadio.parsePacket() == 5) &&
      (LoRaRadio.read() == 'S') &&
      (LoRaRadio.read() == 'E') &&
      (LoRaRadio.read() == 'N') &&
      (LoRaRadio.read() == 'D'))
  {
    int pirEvents = LoRaRadio.read();
    Serial.println("Received signal: ");
    Serial.print("Detections: ");
    Serial.println(pirEvents);
    // got signal so send confirm
    
    if (!LoRaRadio.busy()){
      LoRaRadio.beginPacket();
      LoRaRadio.write('O');
      LoRaRadio.write('K');
      LoRaRadio.endPacket();
      Serial.println("sent");
    }
    
    LoRaRadio.receive();
  }
  else{
    LoRaRadio.receive();
  //Serial.println("sent2");
  }
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

  LoRaRadio.onReceive(receiveCallback);
  LoRaRadio.receive(5000);
}

void loop() {
  //LoRaRadio.receive(1000);
}
