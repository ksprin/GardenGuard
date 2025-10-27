#include "LoRaRadio.h"
#include "TimerMillis.h"
#include "STM32L0.h"

int pirStatus;
unsigned char data[4]={};
float distance;

void setup(void) {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //Serial1.begin(9600);
    
  while (!Serial) { }

  pirStatus = 0;
  // set output on PA_9: GPIO out (8) : buzzer in
  pinMode(8, OUTPUT);

  // set interupt on PB_15: GPIO in (11) : PIR output
  pinMode(11, INPUT); 

  // Moisture A0 (16)
  pinMode(16, INPUT); 

  // Moisture Sensor Power: PB_14: GPIO out (12)
  pinMode(12, OUTPUT);

  // User Button
  //pinMode(6, INPUT);

  // LED
  pinMode(PIN_LED2, OUTPUT);

  //attachInterrupt(11, pirInterupt, RISING);
  digitalWrite(PIN_LED2, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  pirStatus = digitalRead(11);
  if (pirStatus == HIGH){
    Serial.println("1");
  }else{
    Serial.println("0");
  }
  /*
  //if (Serial1.available() > 0){
    //Serial.print("loop ");
    do{
      for(int i = 0; i < 4; i++){
        data[i] == Serial1.read();
      }
    }while(Serial1.read()==0xff);
  //}

  Serial1.flush();

  if(data[0] == 0xff){
    int sum;
    sum = (data[0]+data[1]+data[2])&0x00FF;
    if(sum==data[3]){
      distance=(data[1]<<8)+data[2];
      if(distance>280){
        Serial.print("distance ");
        Serial.println(distance/10);
      }else{
        Serial.println("below");
      }
    }else{
      Serial.println("error");
    }
  }
  */

  delay(250);
}
