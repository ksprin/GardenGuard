#include "TimerMillis.h"
#include "STM32L0.h"

void setup() {
  Serial.begin(9600);

  while (!Serial) { }

  // Moisture A0 (16)
  pinMode(16, INPUT); 

  // Power: PB_14: GPIO out (12)
  pinMode(12, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  int data = analogRead(16);
  Serial.print("Moisture Data: ");
  Serial.println(data);
  delay(5000);

}
