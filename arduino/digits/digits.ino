#include <SPI.h>
#include <HashMap.h>

String serialInput = "";

int dataPin = 9;
int latchPin = 8;
int clockPin = 7;

int count = 0;

unsigned long greenNextStep = 0;
unsigned long redNextStep = 0;
unsigned long ampMeterNextStep = 0;
unsigned long voltMeterNextStep = 0;
unsigned long bigVoltMeterNextStep = 0;
unsigned long countNextStep = 0;

int ampMeterTarget = 0;
int bigVoltMeterTarget = 0;
int voltMeterTarget = 0;

void setup() {
    Serial.begin(9600);
    
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);    
    pinMode(latchPin, OUTPUT);    
}

void loop()  {    
   
   boolean leadingBlank = true;           
   for (int i = 3; i >= 0; i--) {     
      int num = count%10;
       if (i == 1) {
         num = (count/10%10);
       }
       if (i == 2) {
         num = (count/100%10);
       }
       if (i == 3) {
         num = (count/1000%10);
       }
        
       if (num > 0 || i == 0) {
         leadingBlank = false; 
       }
                
       byte digit = num << 4;
       if (!leadingBlank) {
         bitSet(digit, i);
       }
        
       digitalWrite(latchPin, LOW);
       shiftOut(dataPin, clockPin, LSBFIRST, digit);
       digitalWrite(latchPin, HIGH);
       delay(1);         
    }

    count = count + 1;
    readSerial();    
}

void readSerial() {
     String payLoadString = "";

     if (Serial.available() > 0) {
         char incomingByte = Serial.read();
         if (incomingByte == '\n') {
            payLoadString = serialInput;
            serialInput = "";
       } else {
          serialInput.concat(incomingByte);
       }
       
    } else {
       return; 
    }
     
    if(payLoadString.startsWith("count:")) {
         String valueString = payLoadString.substring(6, payLoadString.length());
         count = stringToInt(valueString);         
    }  
    
}

int stringToInt(String value) {
    char numbers[100];
    value.toCharArray(numbers, value.length() + 1);
    int dest = atoi(numbers);
    return dest;
}
