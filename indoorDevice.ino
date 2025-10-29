#include "LoRaRadio.h"
#include "TimerMillis.h"
#include <EEPROM.h>

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
char readVal;

// for summary: EEPROM len 4096 (send dry, wet, buzzerState, buzzerOverride from data point)
int buildingSummary; // 1 or 0
int tempAvg; // addr 0
int tempMin; // addr 1
int tempMax; // addr 2
int moistureAvg; // addr 3
int moistureMin; // addr 4
int moistureMax; // addr 5
int numOfDetec; // addr 6
int numOfDatapoints; // addr 7
int estabMinMax; // addr 8

int needToCommunicate;



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

    LoRaRadio.beginPacket();
    LoRaRadio.write('O');
    LoRaRadio.write('K');
    LoRaRadio.write(dataComputer);
    LoRaRadio.endPacket();
    
    // check if using summary
    if (buildingSummary){
      // need to send summary if reply
      Serial.println("s");

      delay(2000);
      if (Serial.available() > 0){
        readVal = Serial.read(); // clear the buffer
        
        // set flags for summary and send
        needToCommunicate = 1;
        buildingSummary = 1;
      }
      else{
        // set flags for summary
        buildingSummary = 1;
        needToCommunicate = 0;
      }
    }
    else{
      Serial.println("d");
      // just send data if reply
      delay(2000);
      if (Serial.available() > 0){
        readVal = Serial.read(); // clear the buffer
        
        // set flags for send
        needToCommunicate = 1;
        buildingSummary = 0;
      }
      else{
        // set flags for summary
        buildingSummary = 1;
        needToCommunicate = 0;
      }
    }

    // write to mem is needed
    if (buildingSummary){
      // add to summary
      tempAvg = EEPROM.read(0); // addr 0
      tempMin = EEPROM.read(1);// addr 1
      tempMax = EEPROM.read(2); // addr 2
      moistureAvg = EEPROM.read(3); // addr 3
      moistureMin = EEPROM.read(4); // addr 4
      moistureMax = EEPROM.read(5); // addr 5
      numOfDetec = EEPROM.read(6); // addr 6
      numOfDatapoints = EEPROM.read(7); // addr 7
      estabMinMax = EEPROM.read(8); // addr 8

      numOfDetec = numOfDetec + pirEvents;
      numOfDatapoints = numOfDatapoints + 1;
      if (tempAvg == 0){
        tempAvg = temperature;
      }
      else{
        tempAvg = tempAvg * (numOfDatapoints -1)/numOfDatapoints + temperature / numOfDatapoints;
      }
      if (moistureAvg == 0){
        moistureAvg = moistureLevel;
      }
      else{
        moistureAvg = moistureAvg * (numOfDatapoints -1)/numOfDatapoints + moistureLevel / numOfDatapoints;
      }

      if (estabMinMax == 0){
        // min and max are null so put in intit values
        tempMin = temperature;
        tempMax = temperature;
        moistureMin = moistureLevel;
        moistureMax = moistureLevel;
      }
      else{
        // min and max can be updated
        if (temperature < tempMin){
          tempMin = temperature;
        }
        else if (temperature > tempMax){
          tempMax = temperature;
        }

        if (moistureLevel < moistureMin){
          moistureMin = moistureLevel;
        }
        else if (moistureLevel > moistureMax){
          moistureMax = moistureLevel;
        }
      }

      // write to memory
      EEPROM.write(0, tempAvg); // addr 0
      EEPROM.write(1, tempMin);// addr 1
      EEPROM.write(2, tempMax); // addr 2
      EEPROM.write(3, moistureAvg); // addr 3
      EEPROM.write(4, moistureMin); // addr 4
      EEPROM.write(5, moistureMax); // addr 5
      EEPROM.write(6, numOfDetec); // addr 6
      EEPROM.write(7, numOfDatapoints); // addr 7
      EEPROM.write(8, 1); // add 8, min and max have values
    }
    if (needToCommunicate){
      if (buildingSummary){
        // send summary
        Serial.println(","+String(moistureLevel)+","+String(temperature)+","+String(pirEvents)+","+String(buzzerState)+","+String(buzzerOverride)+","+String(dry)+","+String(wet)+","+String(tempAvg)+","+String(tempMin)+","+String(tempMax)+","+String(moistureAvg)+","+String(moistureMin)+","+String(moistureMax)+","+String(numOfDetec)+","+String(numOfDatapoints));

        // communicated with computer: clear summary and set to false
        buildingSummary = 0;

        EEPROM.write(0, 0); // addr 0
        EEPROM.write(1, 0);// addr 1
        EEPROM.write(2, 0); // addr 2
        EEPROM.write(3, 0); // addr 3
        EEPROM.write(4, 0); // addr 4
        EEPROM.write(5, 0); // addr 5
        EEPROM.write(6, 0); // addr 6
        EEPROM.write(7, 0); // addr 7
        EEPROM.write(8, 0); // addr 8, the min and max are null again
      }
      else{
        // send stnd data
        Serial.println(","+String(moistureLevel)+","+String(temperature)+","+String(pirEvents)+","+String(buzzerState)+","+String(buzzerOverride)+","+String(dry)+","+String(wet));
      }

    }
    
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

  // set up memory system
  buildingSummary = 0; // 1 or 0
  needToCommunicate = 0; // 1 or 0
  tempAvg = EEPROM.read(0); // addr 0
  tempMin = EEPROM.read(1);// addr 1
  tempMax = EEPROM.read(2); // addr 2
  moistureAvg = EEPROM.read(3); // addr 3
  moistureMin = EEPROM.read(4); // addr 4
  moistureMax = EEPROM.read(5); // addr 5
  numOfDetec = EEPROM.read(6); // addr 6
  numOfDatapoints = EEPROM.read(7); // addr 7
  estabMinMax = EEPROM.read(8); // addr 8

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
