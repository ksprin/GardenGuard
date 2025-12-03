#include "LoRaRadio.h"
#include "TimerMillis.h"
#include <EEPROM.h>

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
int buzzerState = 0;
int buzzerOverride = 0;
int dry = 0;
int wet = 100;

int waitDataTime = 150;

char dataComputer = 'x';
char readVal;

// for summary: EEPROM len 4096 (send dry, wet, buzzerState, buzzerOverride from data point)
int buildingSummary; // 1 or 0

//long is 4 bytes
//float is 4 bytes
// int is 4 bytes

int tempAvg_b1_addr = 0;
int tempAvg_b2_addr = 1;
int tempAvg_b3_addr = 2;
int tempAvg_b4_addr = 3;

union {
  byte as_byte[4];
  float as_float;
} tempAvg;

int tempMin_b1_addr = 4;
int tempMin_b2_addr = 5;
int tempMin_b3_addr = 6;
int tempMin_b4_addr = 7;

union {
  byte as_byte[4];
  float as_float;
} tempMin;

int tempMax_b1_addr = 8;
int tempMax_b2_addr = 9;
int tempMax_b3_addr = 10;
int tempMax_b4_addr = 11;

union {
  byte as_byte[4];
  float as_float;
} tempMax;

int numOfDatapoints_b1_addr = 12;
int numOfDatapoints_b2_addr = 13;
int numOfDatapoints_b3_addr = 14;
int numOfDatapoints_b4_addr = 15;

union {
  byte as_byte[4];
  long as_long;
} numOfDatapoints;

int numOfDetec_b1_addr = 16;
int numOfDetec_b2_addr = 17;
int numOfDetec_b3_addr = 18;
int numOfDetec_b4_addr = 19;

union {
  byte as_byte[4];
  int as_int;
} numOfDetec;

int moistureAvg_b1_addr = 20;
int moistureAvg_b2_addr = 21;
int moistureAvg_b3_addr = 22;
int moistureAvg_b4_addr = 23;

union {
  byte as_byte[4];
  int as_int;
} moistureAvg;

int moistureMin_b1_addr = 24;
int moistureMin_b2_addr = 25;
int moistureMin_b3_addr = 26;
int moistureMin_b4_addr = 27;

union {
  byte as_byte[4];
  int as_int;
} moistureMin;

int moistureMax_b1_addr = 28;
int moistureMax_b2_addr = 29;
int moistureMax_b3_addr = 30;
int moistureMax_b4_addr = 31;

union {
  byte as_byte[4];
  int as_int;
} moistureMax;

int estabMinMax; 
int estabMinMax_addr = 32;

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
  LoRaRadio.setSpreadingFactor(LoRaRadio.SF_12);
  LoRaRadio.setCodingRate(LoRaRadio.CR_4_5);
  LoRaRadio.setLnaBoost(true);

  // set up memory system
  buildingSummary = 1; // 1 or 0
  needToCommunicate = 0; // 1 or 0
  readBuzzerFlag = 0;
  receiveFlag = 0;
  checkIfThereFlag = 0;
  
  tempAvg.as_byte[0] = EEPROM.read(tempAvg_b1_addr);
  tempAvg.as_byte[1] = EEPROM.read(tempAvg_b2_addr);
  tempAvg.as_byte[2]= EEPROM.read(tempAvg_b3_addr);
  tempAvg.as_byte[3] = EEPROM.read(tempAvg_b4_addr);

  tempMin.as_byte[0] = EEPROM.read(tempMin_b1_addr);
  tempMin.as_byte[1] = EEPROM.read(tempMin_b2_addr);
  tempMin.as_byte[2] = EEPROM.read(tempMin_b3_addr);
  tempMin.as_byte[3] = EEPROM.read(tempMin_b4_addr);

  tempMax.as_byte[0] = EEPROM.read(tempMax_b1_addr);
  tempMax.as_byte[1] = EEPROM.read(tempMax_b2_addr);
  tempMax.as_byte[2] = EEPROM.read(tempMax_b3_addr);
  tempMax.as_byte[3] = EEPROM.read(tempMax_b4_addr);
  
  numOfDatapoints.as_byte[0] = EEPROM.read(numOfDatapoints_b1_addr);
  numOfDatapoints.as_byte[1] = EEPROM.read(numOfDatapoints_b2_addr);
  numOfDatapoints.as_byte[2] = EEPROM.read(numOfDatapoints_b3_addr);
  numOfDatapoints.as_byte[3] = EEPROM.read(numOfDatapoints_b4_addr);

  moistureAvg.as_byte[0] = EEPROM.read(moistureAvg_b1_addr);
  moistureAvg.as_byte[1] = EEPROM.read(moistureAvg_b2_addr);
  moistureAvg.as_byte[2] = EEPROM.read(moistureAvg_b3_addr);
  moistureAvg.as_byte[3] = EEPROM.read(moistureAvg_b4_addr);

  moistureMin.as_byte[0] = EEPROM.read(moistureMin_b1_addr);
  moistureMin.as_byte[1] = EEPROM.read(moistureMin_b2_addr);
  moistureMin.as_byte[2] = EEPROM.read(moistureMin_b3_addr);
  moistureMin.as_byte[3] = EEPROM.read(moistureMin_b4_addr);

  moistureMax.as_byte[0] = EEPROM.read(moistureMax_b1_addr);
  moistureMax.as_byte[1] = EEPROM.read(moistureMax_b2_addr);
  moistureMax.as_byte[2] = EEPROM.read(moistureMax_b3_addr);
  moistureMax.as_byte[3] = EEPROM.read(moistureMax_b4_addr);

  numOfDetec.as_byte[0] = EEPROM.read(numOfDetec_b1_addr);
  numOfDetec.as_byte[1] = EEPROM.read(numOfDetec_b2_addr);
  numOfDetec.as_byte[2] = EEPROM.read(numOfDetec_b3_addr);
  numOfDetec.as_byte[3] = EEPROM.read(numOfDetec_b4_addr);

  estabMinMax = EEPROM.read(estabMinMax_addr);

  LoRaRadio.onTransmit(transmitCallback);
  LoRaRadio.onReceive(receiveCallback);

  readFromComputerClock.start(readFromComputerClockHandler, 15000, 20000);
  checkIfThere.start(checkIfThereClockHandler, 5000, 5000);
  LoRaRadio.receive(500);

}

void loop() {
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
        delay(50);
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
      
      checkIfThere.start(checkIfThereClockHandler, 5000, 5000);
      checkIfThereFlag = 1;
    }
    readBuzzerFlag = 0;
  }

  if (receiveFlag && !checkIfThereFlag){
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
    
    tempAvg.as_byte[0] = EEPROM.read(tempAvg_b1_addr);
    tempAvg.as_byte[1] = EEPROM.read(tempAvg_b2_addr);
    tempAvg.as_byte[2]= EEPROM.read(tempAvg_b3_addr);
    tempAvg.as_byte[3] = EEPROM.read(tempAvg_b4_addr);

    tempMin.as_byte[0] = EEPROM.read(tempMin_b1_addr);
    tempMin.as_byte[1] = EEPROM.read(tempMin_b2_addr);
    tempMin.as_byte[2] = EEPROM.read(tempMin_b3_addr);
    tempMin.as_byte[3] = EEPROM.read(tempMin_b4_addr);

    tempMax.as_byte[0] = EEPROM.read(tempMax_b1_addr);
    tempMax.as_byte[1] = EEPROM.read(tempMax_b2_addr);
    tempMax.as_byte[2] = EEPROM.read(tempMax_b3_addr);
    tempMax.as_byte[3] = EEPROM.read(tempMax_b4_addr);
    
    numOfDatapoints.as_byte[0] = EEPROM.read(numOfDatapoints_b1_addr);
    numOfDatapoints.as_byte[1] = EEPROM.read(numOfDatapoints_b2_addr);
    numOfDatapoints.as_byte[2] = EEPROM.read(numOfDatapoints_b3_addr);
    numOfDatapoints.as_byte[3] = EEPROM.read(numOfDatapoints_b4_addr);

    moistureAvg.as_byte[0] = EEPROM.read(moistureAvg_b1_addr);
    moistureAvg.as_byte[1] = EEPROM.read(moistureAvg_b2_addr);
    moistureAvg.as_byte[2] = EEPROM.read(moistureAvg_b3_addr);
    moistureAvg.as_byte[3] = EEPROM.read(moistureAvg_b4_addr);

    moistureMin.as_byte[0] = EEPROM.read(moistureMin_b1_addr);
    moistureMin.as_byte[1] = EEPROM.read(moistureMin_b2_addr);
    moistureMin.as_byte[2] = EEPROM.read(moistureMin_b3_addr);
    moistureMin.as_byte[3] = EEPROM.read(moistureMin_b4_addr);

    moistureMax.as_byte[0] = EEPROM.read(moistureMax_b1_addr);
    moistureMax.as_byte[1] = EEPROM.read(moistureMax_b2_addr);
    moistureMax.as_byte[2] = EEPROM.read(moistureMax_b3_addr);
    moistureMax.as_byte[3] = EEPROM.read(moistureMax_b4_addr);

    numOfDetec.as_byte[0] = EEPROM.read(numOfDetec_b1_addr);
    numOfDetec.as_byte[1] = EEPROM.read(numOfDetec_b2_addr);
    numOfDetec.as_byte[2] = EEPROM.read(numOfDetec_b3_addr);
    numOfDetec.as_byte[3] = EEPROM.read(numOfDetec_b4_addr);

    estabMinMax = EEPROM.read(estabMinMax_addr);

    numOfDetec.as_int = numOfDetec.as_int + pirEvents;
    numOfDatapoints.as_long = numOfDatapoints.as_long + 1;

    if (estabMinMax == 0){
      // min and max are null so put in intit values
      tempMin.as_float = (int)temperature;
      tempMax.as_float = (int)temperature;
      moistureMin.as_int = moistureLevel;
      moistureMax.as_int = moistureLevel;
      tempAvg.as_float = (int)temperature;
      moistureAvg.as_int = moistureLevel;

      estabMinMax = 1;
    }
    else{
      // min and max can be updated
      if (temperature < tempMin.as_float){
        tempMin.as_float = temperature;
      }
      if (temperature > tempMax.as_float){
        tempMax.as_float = temperature;
      }

      if (moistureLevel < moistureMin.as_int){
        moistureMin.as_int = moistureLevel;
      }
      else if (moistureLevel > moistureMax.as_int){
        moistureMax.as_int = moistureLevel;
      }
    }
    
    if (tempAvg.as_float == 0){
      tempAvg.as_float = temperature;
    }
    else{
      tempAvg.as_float = tempAvg.as_float * ((float)numOfDatapoints.as_long -1)/(float)numOfDatapoints.as_long + temperature / (float)numOfDatapoints.as_long;
      if (tempAvg.as_float == 0){
        tempAvg.as_float = temperature;
      }
    }
    if (moistureAvg.as_byte == 0){
      moistureAvg.as_int = moistureLevel;
    }
    else{
      moistureAvg.as_int = round((float)moistureAvg.as_int * ((float)numOfDatapoints.as_long -1)/(float)numOfDatapoints.as_long + (float)moistureLevel / (float)numOfDatapoints.as_long);
      if (moistureAvg.as_int == 0){
        moistureAvg.as_int = moistureLevel;
      }
    }

    // write to memory
    
    EEPROM.write(tempAvg_b1_addr, tempAvg.as_byte[0]);
    EEPROM.write(tempAvg_b2_addr, tempAvg.as_byte[1]);
    EEPROM.write(tempAvg_b3_addr, tempAvg.as_byte[2]);
    EEPROM.write(tempAvg_b4_addr, tempAvg.as_byte[3]);

    EEPROM.write(tempMin_b1_addr, tempMin.as_byte[0]);
    EEPROM.write(tempMin_b2_addr, tempMin.as_byte[1]);
    EEPROM.write(tempMin_b3_addr, tempMin.as_byte[2]);
    EEPROM.write(tempMin_b4_addr, tempMin.as_byte[3]);

    EEPROM.write(tempMax_b1_addr, tempMax.as_byte[0]);
    EEPROM.write(tempMax_b2_addr, tempMax.as_byte[1]);
    EEPROM.write(tempMax_b3_addr, tempMax.as_byte[2]);
    EEPROM.write(tempMax_b4_addr, tempMax.as_byte[3]);

    EEPROM.write(numOfDatapoints_b1_addr, numOfDatapoints.as_byte[0]);
    EEPROM.write(numOfDatapoints_b2_addr, numOfDatapoints.as_byte[1]);
    EEPROM.write(numOfDatapoints_b3_addr, numOfDatapoints.as_byte[2]);
    EEPROM.write(numOfDatapoints_b4_addr, numOfDatapoints.as_byte[3]);

    EEPROM.write(moistureAvg_b1_addr, moistureAvg.as_byte[0]);
    EEPROM.write(moistureAvg_b2_addr, moistureAvg.as_byte[1]);
    EEPROM.write(moistureAvg_b3_addr, moistureAvg.as_byte[2]);
    EEPROM.write(moistureAvg_b4_addr, moistureAvg.as_byte[3]);

    EEPROM.write(moistureMin_b1_addr, moistureMin.as_byte[0]);
    EEPROM.write(moistureMin_b2_addr, moistureMin.as_byte[1]);
    EEPROM.write(moistureMin_b3_addr, moistureMin.as_byte[2]);
    EEPROM.write(moistureMin_b4_addr, moistureMin.as_byte[3]);

    EEPROM.write(moistureMax_b1_addr, moistureMax.as_byte[0]);
    EEPROM.write(moistureMax_b2_addr, moistureMax.as_byte[1]);
    EEPROM.write(moistureMax_b3_addr, moistureMax.as_byte[2]);
    EEPROM.write(moistureMax_b4_addr, moistureMax.as_byte[3]);

    EEPROM.write(numOfDetec_b1_addr, numOfDetec.as_byte[0]);
    EEPROM.write(numOfDetec_b2_addr, numOfDetec.as_byte[1]);
    EEPROM.write(numOfDetec_b3_addr, numOfDetec.as_byte[2]);
    EEPROM.write(numOfDetec_b4_addr, numOfDetec.as_byte[3]);
 
    EEPROM.write(estabMinMax_addr, estabMinMax); // add 8, min and max have values

  }
  if (receiveFlag && needToCommunicate){
    if (buildingSummary){
      // send summary
      Serial.println("[GardenGuard]s|"+String(moistureLevel)+","+String(temperature)+","+String(pirEvents)+","+String(buzzerState)+","+String(buzzerOverride)+","+String(dry)+","+String(wet)+","+String(tempAvg.as_float)+","+String(tempMin.as_float)+","+String(tempMax.as_float)+","+String(moistureAvg.as_int)+","+String(moistureMin.as_int)+","+String(moistureMax.as_int)+","+String(numOfDetec.as_int)+","+String(numOfDatapoints.as_long));

      // communicated with computer: clear summary and set to false
      buildingSummary = 0;

      for (int i = 0; i <= 32; i++){
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
