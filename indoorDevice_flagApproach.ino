#include "LoRaRadio.h"
#include "TimerMillis.h"
#include <EEPROM.h>

// maybe use clock interrupt to send last data to computer (serial output)
// maybe use clock interrupt to listen to computer

// rework to maybe not have delay in interupt, create finite state machine
// switch to flag system with interupts

TimerMillis writeToComputerClock;
TimerMillis readFromComputerClock;

char temp1 = 0;
char temp2 = 0;
int rVal = 0;
float vsig = 0;

int pirEvents = 0;
long temperature = 0;
int moistureLevel = 0;
int buzzerState = 0; // init on 0: buzzer off
int buzzerOverride = 0;
int dry = 0;
int wet = 100;

char dataComputer = 'x';
char readVal;

// for summary: EEPROM len 4096 (send dry, wet, buzzerState, buzzerOverride from data point)
int buildingSummary; // 1 or 0

// need to break down tempAvg into bytes: long is 4 bytes
long tempAvg; // addr 0
long tempMin; // addr 1
long tempMax; // addr 2

char tempAvg_b1;
int tempAvg_b1_addr = 0;
char tempAvg_b2;
int tempAvg_b2_addr = 1;
char tempAvg_b3;
int tempAvg_b3_addr = 2;
char tempAvg_b4;
int tempAvg_b4_addr = 3;

char tempMin_b1;
int tempMin_b1_addr = 4;
char tempMin_b2;
int tempMin_b2_addr = 5;
char tempMin_b3;
int tempMin_b3_addr = 6;
char tempMin_b4;
int tempMin_b4_addr = 7;

char tempMax_b1;
int tempMax_b1_addr = 8;
char tempMax_b2;
int tempMax_b2_addr = 9;
char tempMax_b3;
int tempMax_b3_addr = 10;
char tempMax_b4;
int tempMax_b4_addr = 11;

int moistureAvg; 
int moistureAvg_addr = 12;

int moistureMin; 
int moistureMin_addr = 13;

int moistureMax; 
int moistureMax_addr = 14;

int numOfDetec; 
int numOfDetec_addr = 15;

int numOfDatapoints; 
int numOfDatapoints_addr = 16;

int estabMinMax; 
int estabMinMax_addr = 17;

int dataPointForAvg;
int dataPointForAvg_addr = 18;

int needToCommunicate;

int readBuzzerFlag; // flag to know if when to communicate with computer about buzzer
int receiveFlag; // flag to know if something was received
// Char codes
// x - null
// r - read from computer serial port
// n - no read

void readFromComputerClockHandler(void){
  readBuzzerFlag = 1;
}

static void receiveCallback(void){
  if ((LoRaRadio.parsePacket() == 10) &&
      (LoRaRadio.read() == 'S') &&
      (LoRaRadio.read() == 'P'))
  {
    pirEvents = LoRaRadio.read();
    moistureLevel = LoRaRadio.read();
    temp1 = LoRaRadio.read();
    temp2 = LoRaRadio.read();
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
    
    rVal = 0;
    rVal |= ((int)temp1) << 8;
    rVal |= ((int)temp2);

    vsig = (3.3/1023)*rVal;
    temperature = (vsig)/((3.3 - vsig)/10000);
    receiveFlag = 1;
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
  readBuzzerFlag = 0;
  receiveFlag = 0;
  
  tempAvg_b1 = EEPROM.read(tempAvg_b1_addr);
  tempAvg_b2 = EEPROM.read(tempAvg_b2_addr);
  tempAvg_b3 = EEPROM.read(tempAvg_b3_addr);
  tempAvg_b4 = EEPROM.read(tempAvg_b4_addr);

  tempMin_b1 = EEPROM.read(tempMin_b1_addr);
  tempMin_b2 = EEPROM.read(tempMin_b2_addr);
  tempMin_b3 = EEPROM.read(tempMin_b3_addr);
  tempMin_b4 = EEPROM.read(tempMin_b4_addr);

  tempMax_b1 = EEPROM.read(tempMax_b1_addr);
  tempMax_b2 = EEPROM.read(tempMax_b2_addr);
  tempMax_b3 = EEPROM.read(tempMax_b3_addr);
  tempMax_b4 = EEPROM.read(tempMax_b4_addr);

  moistureAvg = EEPROM.read(moistureAvg_addr);
  moistureMin = EEPROM.read(moistureMin_addr);
  moistureMax = EEPROM.read(moistureMax_addr);
  numOfDetec = EEPROM.read(numOfDetec_addr);
  numOfDatapoints = EEPROM.read(numOfDatapoints_addr);
  estabMinMax = EEPROM.read(estabMinMax_addr);

  dataPointForAvg = EEPROM.read(dataPointForAvg_addr);

  // rebuild tempAvg, tempMin, tempMax
  tempAvg = ((long)tempAvg_b1<<24)|((long)tempAvg_b2<<16)|((long)tempAvg_b3<<8)|(long)tempAvg_b4;
  tempMin = ((long)tempMin_b1<<24)|((long)tempMin_b2<<16)|((long)tempMin_b3<<8)|(long)tempMin_b4;
  tempMax = ((long)tempMax_b1<<24)|((long)tempMax_b2<<16)|((long)tempMax_b3<<8)|(long)tempMax_b4;

  LoRaRadio.onTransmit(transmitCallback);
  LoRaRadio.onReceive(receiveCallback);

  //writeToComputerClock.start(writeToComputerClockHandler, 5000, 5000);
  readFromComputerClock.start(readFromComputerClockHandler, 15000, 10000);

  LoRaRadio.receive(500); // might be a concern

}

void loop() {
  // all writing and waiting for computer

  // For resetting buzzer
  if (readBuzzerFlag){
    // Clear buffer
    while (Serial.available() > 0){
      Serial.read();
    }

    // send string to indicate receiving
    Serial.println("r");
    delay(1000);
    if (Serial.available() > 0){
      dataComputer = Serial.read();
      Serial.println(dataComputer); // confirm
    }
    else{
      Serial.println("n");
    }
    readBuzzerFlag = 0;
  }

  if (receiveFlag){
    // disable lora: need to minimize wait time
    LoRaRadio.receive(1500);
    //LoRaRadio.end();
    // check if using summary
    if (buildingSummary){
      // Clear buffer
      while (Serial.available() > 0){
        Serial.read();
      }

      // need to send summary if reply
      Serial.println("s");

      delay(1000);
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
      // Clear buffer
      while (Serial.available() > 0){
        Serial.read();
      }

      Serial.println("d");
      // just send data if reply
      delay(1000);
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
      tempAvg_b1 = EEPROM.read(tempAvg_b1_addr);
      tempAvg_b2 = EEPROM.read(tempAvg_b2_addr);
      tempAvg_b3 = EEPROM.read(tempAvg_b3_addr);
      tempAvg_b4 = EEPROM.read(tempAvg_b4_addr);

      tempMin_b1 = EEPROM.read(tempMin_b1_addr);
      tempMin_b2 = EEPROM.read(tempMin_b2_addr);
      tempMin_b3 = EEPROM.read(tempMin_b3_addr);
      tempMin_b4 = EEPROM.read(tempMin_b4_addr);

      tempMax_b1 = EEPROM.read(tempMax_b1_addr);
      tempMax_b2 = EEPROM.read(tempMax_b2_addr);
      tempMax_b3 = EEPROM.read(tempMax_b3_addr);
      tempMax_b4 = EEPROM.read(tempMax_b4_addr);

      moistureAvg = EEPROM.read(moistureAvg_addr);
      moistureMin = EEPROM.read(moistureMin_addr);
      moistureMax = EEPROM.read(moistureMax_addr);
      numOfDetec = EEPROM.read(numOfDetec_addr); 
      numOfDatapoints = EEPROM.read(numOfDatapoints_addr); 
      estabMinMax = EEPROM.read(estabMinMax_addr);
      dataPointForAvg = EEPROM.read(dataPointForAvg_addr);

      // rebuild tempAvg, tempMin, tempMax
      tempAvg = ((long)tempAvg_b1<<24)|((long)tempAvg_b2<<16)|((long)tempAvg_b3<<8)|(long)tempAvg_b4;
      tempMin = ((long)tempMin_b1<<24)|((long)tempMin_b2<<16)|((long)tempMin_b3<<8)|(long)tempMin_b4;
      tempMax = ((long)tempMax_b1<<24)|((long)tempMax_b2<<16)|((long)tempMax_b3<<8)|(long)tempMax_b4;

      numOfDetec = numOfDetec + pirEvents;
      numOfDatapoints = numOfDatapoints + 1;

      //only add to avg if detected
      if (pirEvents > 0){
        dataPointForAvg = dataPointForAvg + 1;
        if (tempAvg == 0){
          tempAvg = temperature;
        }
        else{
          tempAvg = tempAvg * (dataPointForAvg -1)/dataPointForAvg + temperature / dataPointForAvg;
        }
        if (moistureAvg == 0){
          moistureAvg = moistureLevel;
        }
        else{
          moistureAvg = moistureAvg * (dataPointForAvg -1)/dataPointForAvg + moistureLevel / dataPointForAvg;
        }
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

      // break down tempAvg, tempMin, tempMax
      tempAvg_b1 = (tempAvg >> 24);
      tempAvg_b2 = (tempAvg >> 16);
      tempAvg_b3 = (tempAvg >> 8);
      tempAvg_b4 = (tempAvg);

      tempMin_b1 = (tempMin >> 24);
      tempMin_b2 = (tempMin >> 16);
      tempMin_b3 = (tempMin >> 8);
      tempMin_b4 = (tempMin);

      tempMax_b1 = (tempMax >> 24);
      tempMax_b2 = (tempMax >> 16);
      tempMax_b3 = (tempMax >> 8);
      tempMax_b4 = (tempMax);

      // write to memory
      EEPROM.write(tempAvg_b1_addr, tempAvg_b1);
      EEPROM.write(tempAvg_b2_addr, tempAvg_b2);
      EEPROM.write(tempAvg_b3_addr, tempAvg_b3);
      EEPROM.write(tempAvg_b4_addr, tempAvg_b4);

      EEPROM.write(tempMin_b1_addr, tempMin_b1);
      EEPROM.write(tempMin_b2_addr, tempMin_b2);
      EEPROM.write(tempMin_b3_addr, tempMin_b3);
      EEPROM.write(tempMin_b4_addr, tempMin_b4);

      EEPROM.write(tempMax_b1_addr, tempMax_b1);
      EEPROM.write(tempMax_b2_addr, tempMax_b2);
      EEPROM.write(tempMax_b3_addr, tempMax_b3);
      EEPROM.write(tempMax_b4_addr, tempMax_b4);

      EEPROM.write(moistureAvg_addr, moistureAvg); // addr 3
      EEPROM.write(moistureMin_addr, moistureMin); // addr 4
      EEPROM.write(moistureMax_addr, moistureMax); // addr 5
      EEPROM.write(numOfDetec_addr, numOfDetec); // addr 6
      EEPROM.write(numOfDatapoints_addr, numOfDatapoints); // addr 7
      EEPROM.write(estabMinMax_addr, 1); // add 8, min and max have values
      EEPROM.write(dataPointForAvg_addr, dataPointForAvg);
    }
    if (needToCommunicate){
      if (buildingSummary){
        // send summary
        Serial.println(","+String(moistureLevel)+","+String(temperature)+","+String(pirEvents)+","+String(buzzerState)+","+String(buzzerOverride)+","+String(dry)+","+String(wet)+","+String(tempAvg)+","+String(tempMin)+","+String(tempMax)+","+String(moistureAvg)+","+String(moistureMin)+","+String(moistureMax)+","+String(numOfDetec)+","+String(numOfDatapoints));

        // communicated with computer: clear summary and set to false
        buildingSummary = 0;

        for (int i = 0; i <= 17; i++){
          EEPROM.write(i, 0);
        }
      }
      else{
        // send stnd data
        Serial.println(","+String(moistureLevel)+","+String(temperature)+","+String(pirEvents)+","+String(buzzerState)+","+String(buzzerOverride)+","+String(dry)+","+String(wet));
      }

    }
    receiveFlag = 0;
  }
}
