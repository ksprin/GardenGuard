#include "LoRaRadio.h"
#include "TimerMillis.h"
#include <EEPROM.h>
//#include <ArduinoSTL.h>
//#include <map>

// maybe use clock interrupt to send last data to computer (serial output)
// maybe use clock interrupt to listen to computer

// rework to maybe not have delay in interupt, create finite state machine
// switch to flag system with interupts

// new commands: send s more often

TimerMillis writeToComputerClock;
TimerMillis readFromComputerClock;
TimerMillis checkIfThere;
char temp1 = 0;
char temp2 = 0;
int rVal = 0;
float RT0 = 10000;
float B = 3977;
float R = 10000;
float T0 = 298.15;
long RT = 0;
float VR = 0.0;
float ln = 0.0;
float TX = 0.0;
float VRT = 0.0;

int pirEvents = 0;
float temperature = 0.0;
int moistureLevel = 0;
int buzzerState = 0; // init on 0: buzzer off
int buzzerOverride = 0;
int dry = 0;
int wet = 100;

int waitDataTime = 150;

char dataComputer = 'x';
char readVal;

// for summary: EEPROM len 4096 (send dry, wet, buzzerState, buzzerOverride from data point)
int buildingSummary; // 1 or 0

// need to break down tempAvg into bytes: long is 4 bytes
int tempAvg; // addr 0
int tempMin; // addr 1
int tempMax; // addr 2

//long is 4 bytes
//float is 4 bytes
/*
//char tempAvg_b1;
int tempAvg_b1_addr = 0;
//char tempAvg_b2;
int tempAvg_b2_addr = 1;
//char tempAvg_b3;
int tempAvg_b3_addr = 2;
//char tempAvg_b4;
int tempAvg_b4_addr = 3;

union {
  byte as_byte[4];
  float as_float;
} tempAvg_union;

//char tempMin_b1;
int tempMin_b1_addr = 4;
//char tempMin_b2;
int tempMin_b2_addr = 5;
//char tempMin_b3;
int tempMin_b3_addr = 6;
//har tempMin_b4;
int tempMin_b4_addr = 7;

union {
  byte as_byte[4];
  float as_float;
} tempMin_union;

//char tempMax_b1;
int tempMax_b1_addr = 8;
//char tempMax_b2;
int tempMax_b2_addr = 9;
//char tempMax_b3;
int tempMax_b3_addr = 10;
//char tempMax_b4;
int tempMax_b4_addr = 11;

union {
  byte as_byte[4];
  float as_float;
} tempMax_union;
*/

int tempAvg_addr = 1;

int tempMin_addr = 2;

int tempMax_addr = 3;

//int numOfDatapoints; 
//int numOfDatapoints_spacesMemTotal = 1;
//int numOfDatapoints_spacesMemTotal_addr = 9;
//int numOfDatapoints_spacesMemMAX =  EEPROM.length() - 1;
//int numOfDatapoints_start_addr = 10;

int numOfDatapoints_b1_addr = 9;
int numOfDatapoints_b2_addr = 10;
int numOfDatapoints_b3_addr = 11;
int numOfDatapoints_b4_addr = 12;

union {
  byte as_byte[4];
  long as_long;
} numOfDatapoints;


int moistureAvg; 
int moistureAvg_addr = 4;

int moistureMin; 
int moistureMin_addr = 5;

int moistureMax; 
int moistureMax_addr = 6;

int numOfDetec; 
int numOfDetec_addr = 7;

int estabMinMax; 
int estabMinMax_addr = 8;

int needToCommunicate;

int moistureRaw = 0;
float moistureLog = 0.0;

int wetRaw = 0;
float wetLog = 0.0;

int dryRaw = 0;
float dryLog = 0.0;


int readBuzzerFlag; // flag to know if when to communicate with computer about buzzer
int receiveFlag; // flag to know if something was received
int checkIfThereFlag;

// Char codes
// x - null
// r - read from computer serial port
// n - no read

void readFromComputerClockHandler(void){
  readBuzzerFlag = 1;
}

void checkIfThereClockHandler(void){
  checkIfThereFlag = 1;
}

static void receiveCallback(void){
  if ((LoRaRadio.parsePacket() == 10) &&
      (LoRaRadio.read() == 'S') &&
      (LoRaRadio.read() == 'P'))
  {
    pirEvents = LoRaRadio.read();
    moistureRaw = LoRaRadio.read();
    temp1 = LoRaRadio.read();
    temp2 = LoRaRadio.read();
    buzzerState = LoRaRadio.read();
    buzzerOverride = LoRaRadio.read();
    wetRaw = LoRaRadio.read();
    dryRaw = LoRaRadio.read();
  
    // got signal so send confirm

    LoRaRadio.beginPacket();
    LoRaRadio.write('O');
    LoRaRadio.write('K');
    LoRaRadio.write(dataComputer);
    LoRaRadio.endPacket();
    
    rVal = 0;
    rVal |= ((int)temp1) << 8;
    rVal |= ((int)temp2);

    VRT = (3.3/1023)*rVal;
    VR = 3.3 - VRT;
    RT = VRT / (VR / R);
    ln = log(RT / RT0);
    TX = (1 / ((ln / B) + (1 / T0)));
    temperature = TX - 273.15;

    //float linVal = pow(2.71828, (float)83/102.3);
    moistureLog = 0.14886*log((float)moistureRaw)+log(49.72607);
    moistureLevel = (int)(pow(2.718, moistureLog));

    dryLog = 0.14886*log((float)dryRaw)+log(49.72607);
    dry = (int)(pow(2.718, dryLog));

    wetLog = 0.14886*log((float)wetRaw)+log(49.72607);
    wet = (int)(pow(2.718, wetLog));

    receiveFlag = 1;
  }
  else{
    LoRaRadio.beginPacket();
    LoRaRadio.write('N');
    LoRaRadio.write('O');
    LoRaRadio.write('x');
    LoRaRadio.endPacket();
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
  buildingSummary = 1; // 1 or 0
  needToCommunicate = 0; // 1 or 0
  readBuzzerFlag = 0;
  receiveFlag = 0;
  checkIfThereFlag = 0;
  /*
  tempAvg_union.as_byte[0] = EEPROM.read(tempAvg_b1_addr);
  tempAvg_union.as_byte[1] = EEPROM.read(tempAvg_b2_addr);
  tempAvg_union.as_byte[2]= EEPROM.read(tempAvg_b3_addr);
  tempAvg_union.as_byte[3] = EEPROM.read(tempAvg_b4_addr);

  tempMin_union.as_byte[0] = EEPROM.read(tempMin_b1_addr);
  tempMin_union.as_byte[1] = EEPROM.read(tempMin_b2_addr);
  tempMin_union.as_byte[2] = EEPROM.read(tempMin_b3_addr);
  tempMin_union.as_byte[3] = EEPROM.read(tempMin_b4_addr);

  tempMax_union.as_byte[0] = EEPROM.read(tempMax_b1_addr);
  tempMax_union.as_byte[1] = EEPROM.read(tempMax_b2_addr);
  tempMax_union.as_byte[2] = EEPROM.read(tempMax_b3_addr);
  tempMax_union.as_byte[3] = EEPROM.read(tempMax_b4_addr);
  */
  numOfDatapoints.as_byte[0] = EEPROM.read(numOfDatapoints_b1_addr);
  numOfDatapoints.as_byte[1] = EEPROM.read(numOfDatapoints_b2_addr);
  numOfDatapoints.as_byte[2] = EEPROM.read(numOfDatapoints_b3_addr);
  numOfDatapoints.as_byte[3] = EEPROM.read(numOfDatapoints_b4_addr);

  tempAvg = EEPROM.read(tempAvg_addr);
  tempMin = EEPROM.read(tempMin_addr);
  tempMax = EEPROM.read(tempMax_addr);

  moistureAvg = EEPROM.read(moistureAvg_addr);
  moistureMin = EEPROM.read(moistureMin_addr);
  moistureMax = EEPROM.read(moistureMax_addr);
  numOfDetec = EEPROM.read(numOfDetec_addr);
  //numOfDatapoints = EEPROM.read(numOfDatapoints_addr);
  estabMinMax = EEPROM.read(estabMinMax_addr);

  // rebuild tempAvg, tempMin, tempMax
  //tempAvg = tempAvg_union.as_float;
  //tempMin = tempMin_union.as_float;
  //tempMax = tempMax_union.as_float;
  //tempAvg = (float)((tempAvg_b1<<24)|(tempAvg_b2<<16)|(tempAvg_b3<<8)|tempAvg_b4);
  //tempMin = (float)((tempMin_b1<<24)|(tempMin_b2<<16)|(tempMin_b3<<8)|tempMin_b4);
  //tempMax = (float)((tempMax_b1<<24)|(tempMax_b2<<16)|(tempMax_b3<<8)|tempMax_b4);
  
  LoRaRadio.onTransmit(transmitCallback);
  LoRaRadio.onReceive(receiveCallback);

  readFromComputerClock.start(readFromComputerClockHandler, 15000, 20000);
  checkIfThere.start(checkIfThereClockHandler, 5000, 5000);
  LoRaRadio.receive(500); // might be a concern

}

void loop() {
  // all writing and waiting for computer
  if (checkIfThereFlag){
    // send check if there
    // Clear buffer
    while (Serial.available() > 0){
      Serial.read();
    }

    // need to send summary if reply
    Serial.println("[GardenGuard]h");
    delay(waitDataTime);
    if (Serial.available() > 0){
      readVal = Serial.read(); // clear the buffer
      if (readVal == 'y'){

      
        // set flags to know the computer was checked for and reoutput the read
        Serial.println("[GardenGuard]h|"+String(readVal));
        checkIfThereFlag = 0;
        // disable check if there clock
        checkIfThere.stop();
        delay(50); // delay after making connection
      }
      else{
        buildingSummary = 1;
        needToCommunicate = 0;
      }
    }
    else{
      buildingSummary = 1;
      needToCommunicate = 0;
    }
  }

  // For resetting buzzer
  if (readBuzzerFlag && !checkIfThereFlag){
    // Clear buffer
    while (Serial.available() > 0){
      Serial.read();
    }

    // send string to indicate receiving
    Serial.println("[GardenGuard]r");
    delay(waitDataTime);
    if (Serial.available() > 0){
      dataComputer = Serial.read();
      Serial.println("[GardenGuard]r|"+String(dataComputer));
    }
    else{
      Serial.println("[GardenGuard]r|n");
      //restart clock to check if there
      buildingSummary = 1;
      needToCommunicate = 0;
      //estabMinMax = 0;
      checkIfThere.start(checkIfThereClockHandler, 5000, 5000);
      checkIfThereFlag = 1;
    }
    readBuzzerFlag = 0;
  }

  if (receiveFlag && !checkIfThereFlag){
    // disable lora: need to minimize wait time
    //LoRaRadio.receive(200);
    //LoRaRadio.end();
    // check if using summary
    if (buildingSummary){
      // Clear buffer
      while (Serial.available() > 0){
        Serial.read();
      }

      // need to send summary if reply
      Serial.println("[GardenGuard]s");

      delay(waitDataTime);
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

        //restart clock to check if there
        checkIfThere.start(checkIfThereClockHandler, 5000, 5000);
        checkIfThereFlag = 1;
      }
    }
    else{
      // Clear buffer
      while (Serial.available() > 0){
        Serial.read();
      }

      Serial.println("[GardenGuard]d");
      // just send data if reply
      delay(waitDataTime);
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

        //restart clock to check if there
        checkIfThere.start(checkIfThereClockHandler, 5000, 5000);
        checkIfThereFlag = 1;
      }
    }
  }

    // write to mem is needed
  if (receiveFlag && buildingSummary){
    // add to summary
    LoRaRadio.receive(75);
    /*
    tempAvg_union.as_byte[0] = EEPROM.read(tempAvg_b1_addr);
    tempAvg_union.as_byte[1] = EEPROM.read(tempAvg_b2_addr);
    tempAvg_union.as_byte[2]= EEPROM.read(tempAvg_b3_addr);
    tempAvg_union.as_byte[3] = EEPROM.read(tempAvg_b4_addr);

    tempMin_union.as_byte[0] = EEPROM.read(tempMin_b1_addr);
    tempMin_union.as_byte[1] = EEPROM.read(tempMin_b2_addr);
    tempMin_union.as_byte[2] = EEPROM.read(tempMin_b3_addr);
    tempMin_union.as_byte[3] = EEPROM.read(tempMin_b4_addr);

    tempMax_union.as_byte[0] = EEPROM.read(tempMax_b1_addr);
    tempMax_union.as_byte[1] = EEPROM.read(tempMax_b2_addr);
    tempMax_union.as_byte[2] = EEPROM.read(tempMax_b3_addr);
    tempMax_union.as_byte[3] = EEPROM.read(tempMax_b4_addr);
    /*
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
    
    tempAvg = tempAvg_union.as_float;
    tempMin = tempMin_union.as_float;
    tempMax = tempMax_union.as_float;
    */
    numOfDatapoints.as_byte[0] = EEPROM.read(numOfDatapoints_b1_addr);
    numOfDatapoints.as_byte[1] = EEPROM.read(numOfDatapoints_b2_addr);
    numOfDatapoints.as_byte[2] = EEPROM.read(numOfDatapoints_b3_addr);
    numOfDatapoints.as_byte[3] = EEPROM.read(numOfDatapoints_b4_addr);

    tempAvg = EEPROM.read(tempAvg_addr);
    tempMin = EEPROM.read(tempMin_addr);
    tempMax = EEPROM.read(tempMax_addr); 

    moistureAvg = EEPROM.read(moistureAvg_addr);
    moistureMin = EEPROM.read(moistureMin_addr);
    moistureMax = EEPROM.read(moistureMax_addr);
    numOfDetec = EEPROM.read(numOfDetec_addr); 
    //numOfDatapoints = EEPROM.read(numOfDatapoints_addr); 
    estabMinMax = EEPROM.read(estabMinMax_addr);

    // rebuild tempAvg, tempMin, tempMax
    //tempAvg = (float)((tempAvg_b1<<24)|(tempAvg_b2<<16)|(tempAvg_b3<<8)|tempAvg_b4);
    //tempMin = (float)((tempMin_b1<<24)|(tempMin_b2<<16)|(tempMin_b3<<8)|tempMin_b4);
    //tempMax = (float)((tempMax_b1<<24)|(tempMax_b2<<16)|(tempMax_b3<<8)|tempMax_b4);

    numOfDetec = numOfDetec + pirEvents;
    numOfDatapoints.as_long = numOfDatapoints.as_long + 1;

    if (estabMinMax == 0){
      // min and max are null so put in intit values
      tempMin = (int)temperature;
      tempMax = (int)temperature;
      moistureMin = moistureLevel;
      moistureMax = moistureLevel;
      tempAvg = (int)temperature;
      moistureAvg = moistureLevel;

      estabMinMax = 1;
    }
    else{
      // min and max can be updated
      if (temperature < tempMin){
        tempMin = (int)temperature;
      }
      if (temperature > tempMax){
        tempMax = (int)temperature;
      }

      if (moistureLevel < moistureMin){
        moistureMin = moistureLevel;
      }
      else if (moistureLevel > moistureMax){
        moistureMax = moistureLevel;
      }
    }
    
    if (tempAvg == 0){
      tempAvg = temperature;
    }
    else{
      tempAvg = round((float)tempAvg * ((float)numOfDatapoints.as_long -1)/(float)numOfDatapoints.as_long + temperature / (float)numOfDatapoints.as_long);
      if (tempAvg == 0){
        tempAvg = (int)temperature;
      }
    }
    if (moistureAvg == 0){
      moistureAvg = moistureLevel;
    }
    else{
      moistureAvg = round((float)moistureAvg * ((float)numOfDatapoints.as_long -1)/(float)numOfDatapoints.as_long + (float)moistureLevel / (float)numOfDatapoints.as_long);
      if (moistureAvg == 0){
        moistureAvg = moistureLevel;
      }
    }
    
    /*
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
    */

    // write to memory
    /*
    EEPROM.write(tempAvg_b1_addr, tempAvg_union.as_byte[0]);
    EEPROM.write(tempAvg_b2_addr, tempAvg_union.as_byte[1]);
    EEPROM.write(tempAvg_b3_addr, tempAvg_union.as_byte[2]);
    EEPROM.write(tempAvg_b4_addr, tempAvg_union.as_byte[3]);

    EEPROM.write(tempMin_b1_addr, tempMin_union.as_byte[0]);
    EEPROM.write(tempMin_b2_addr, tempMin_union.as_byte[1]);
    EEPROM.write(tempMin_b3_addr, tempMin_union.as_byte[2]);
    EEPROM.write(tempMin_b4_addr, tempMin_union.as_byte[3]);

    EEPROM.write(tempMax_b1_addr, tempMax_union.as_byte[0]);
    EEPROM.write(tempMax_b2_addr, tempMax_union.as_byte[1]);
    EEPROM.write(tempMax_b3_addr, tempMax_union.as_byte[2]);
    EEPROM.write(tempMax_b4_addr, tempMax_union.as_byte[3]);
    */
    EEPROM.write(numOfDatapoints_b1_addr, numOfDatapoints.as_byte[0]);
    EEPROM.write(numOfDatapoints_b2_addr, numOfDatapoints.as_byte[1]);
    EEPROM.write(numOfDatapoints_b3_addr, numOfDatapoints.as_byte[2]);
    EEPROM.write(numOfDatapoints_b4_addr, numOfDatapoints.as_byte[3]);

    EEPROM.write(tempAvg_addr, tempAvg);
    EEPROM.write(tempMin_addr, tempMin);
    EEPROM.write(tempMax_addr, tempMax);
    EEPROM.write(moistureAvg_addr, moistureAvg); // addr 3
    EEPROM.write(moistureMin_addr, moistureMin); // addr 4
    EEPROM.write(moistureMax_addr, moistureMax); // addr 5
    EEPROM.write(numOfDetec_addr, numOfDetec); // addr 6
    //EEPROM.write(numOfDatapoints_addr, numOfDatapoints); // addr 7
    EEPROM.write(estabMinMax_addr, estabMinMax); // add 8, min and max have values

  }
  if (receiveFlag && needToCommunicate){
    if (buildingSummary){
      // send summary
      // 0,13625,0,1,1,0,100,0,1481,13957,0,0,5,0,3
      Serial.println("[GardenGuard]s|"+String(moistureLevel)+","+String(temperature)+","+String(pirEvents)+","+String(buzzerState)+","+String(buzzerOverride)+","+String(dry)+","+String(wet)+","+String(tempAvg)+","+String(tempMin)+","+String(tempMax)+","+String(moistureAvg)+","+String(moistureMin)+","+String(moistureMax)+","+String(numOfDetec)+","+String(numOfDatapoints.as_long));

      // communicated with computer: clear summary and set to false
      buildingSummary = 0;

      for (int i = 0; i <= 17; i++){
        EEPROM.write(i, 0);
      }
      estabMinMax = 0;
    }
    else{
      // send stnd data
      Serial.println("[GardenGuard]d|"+String(moistureLevel)+","+String(temperature)+","+String(pirEvents)+","+String(buzzerState)+","+String(buzzerOverride)+","+String(dry)+","+String(wet));
    }

  }
  receiveFlag = 0;
}
