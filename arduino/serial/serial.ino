#include <SPI.h>
#include <HashMap.h>

String serialInput = "";

int dataPin = 9;
int latchPin = 8;
int clockPin = 7;

int bigVoltMeterPin = 3;
int greenPin = 11;
int redPin = 10;
int voltMeterPin = 5;
int ampMeterPin = 6;  // TODO

int greenBrightness = 0;
int redBrightness = 0;
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
int countTarget = 0;

// Remember the current positions on each device
const byte HASH_SIZE = 5; 

HashType<int,int> positionsRawArray[HASH_SIZE];
HashMap<int,int> positions = HashMap<int,int>(positionsRawArray, HASH_SIZE ); 

HashType<int,int> fsdRawArray[HASH_SIZE];
HashMap<int,int> fsds = HashMap<int,int>(fsdRawArray, HASH_SIZE ); 

HashType<int,int> zeroedPinoutsArray[HASH_SIZE];
HashMap<int,int> zeroedPinouts = HashMap<int,int>(zeroedPinoutsArray, HASH_SIZE );

HashType<int,int> fsdPinoutsArray[HASH_SIZE];
HashMap<int,int> fsdPinouts = HashMap<int,int>(fsdPinoutsArray, HASH_SIZE );

void setup() {
    Serial1.begin(9600);
    
    pinMode(greenPin, OUTPUT);    
    pinMode(redPin, OUTPUT);
    pinMode(bigVoltMeterPin, OUTPUT);
    pinMode(voltMeterPin, OUTPUT);
    pinMode(ampMeterPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);    
    pinMode(latchPin, OUTPUT);    
          
    // PWM voltage corresponding to zero and FSD for meters
    zeroedPinouts[0](ampMeterPin, 0);
    zeroedPinouts[1](bigVoltMeterPin, 0);
    zeroedPinouts[2](voltMeterPin, 0);
    zeroedPinouts[3](greenPin, 0);
    zeroedPinouts[4](redPin, 0);

    fsdPinouts[0](ampMeterPin, 220);
    fsdPinouts[1](bigVoltMeterPin, 255);
    fsdPinouts[2](voltMeterPin, 255);
    fsdPinouts[3](greenPin, 255);
    fsdPinouts[4](redPin, 255);

    // The human readable full scale deflection value on the face of each meter
    fsds[0](ampMeterPin, 250);
    fsds[1](bigVoltMeterPin, 100);
    fsds[2](voltMeterPin, 5000);
    fsds[3](greenPin, 255);
    fsds[4](redPin, 255);
    
    // All devices are initially zeroed
    positions[0](ampMeterPin, 0);
    positions[1](bigVoltMeterPin, 0);
    positions[2](voltMeterPin, 0);
    positions[3](greenPin, 0);
    positions[4](redPin, 0);
    
    digitalWrite(greenPin, HIGH);
    delay(1000);
    digitalWrite(greenPin, LOW);
   
    digitalWrite(redPin, HIGH);
    delay(1000);
    digitalWrite(redPin, LOW);    
}

void loop()  {    
   if (millis() > greenNextStep) {
      greenNextStep = millis() + 5;            
      panMeterFromTo(greenPin, greenBrightness);
   }
   
   if (millis() > redNextStep) {
      redNextStep = millis() + 5;       
      panMeterFromTo(redPin, redBrightness);
   }
   
   if (millis() > voltMeterNextStep) {
     voltMeterNextStep = millis() + 20;       
     panMeterFromTo(voltMeterPin, voltMeterTarget);
   }
   
   if (millis() > bigVoltMeterNextStep) {
     bigVoltMeterNextStep = millis() + 50;       
     panMeterFromTo(bigVoltMeterPin, bigVoltMeterTarget);
   }
   
   if (millis() > ampMeterNextStep) {
     ampMeterNextStep = millis() + 20;       
     panMeterFromTo(ampMeterPin, ampMeterTarget);
   }
   
   if (millis() > countNextStep) {
     int countDelay = 100;     
     if (count < countTarget) {
       countDelay = 2000 / (countTarget - count);
       count++;       
     }
     if (count > countTarget) {
       countDelay = 2000 / (count - countTarget);
       count--; 
     }     
     countNextStep = millis() + countDelay;           
   }
   
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

    readSerial();    
}

void readSerial() {
     String payLoadString = "";

     if (Serial1.available() > 0) {
         char incomingByte = Serial1.read();
         if (incomingByte == '\n') {
            payLoadString = serialInput;
            serialInput = "";
       } else {
          serialInput.concat(incomingByte);
       }
       
    } else {
       return; 
    }
    
    if(payLoadString.startsWith("22194:")) {
         String valueString = payLoadString.substring(6, payLoadString.length());
         int dest = stringToInt(valueString);         
         if (dest >= 0 && dest <= fsds.getValueOf(bigVoltMeterPin)) {  // TODO maybe not in the correct place - setMeterTo?         
           bigVoltMeterTarget = setMeterTo(bigVoltMeterPin, dest);           
         } else {                     
           bigVoltMeterTarget = setMeterTo(bigVoltMeterPin, fsds.getValueOf(bigVoltMeterPin));
         }
    }
    
    if(payLoadString.startsWith("22231:")) {
         String valueString = payLoadString.substring(6, payLoadString.length());
         int dest = stringToInt(valueString);
         
         if (dest >= 0 && dest <= fsds.getValueOf(voltMeterPin)) {  // TODO maybe not in the correct place - setMeterTo?         
           voltMeterTarget = setMeterTo(voltMeterPin, dest);           
         } else {                     
           voltMeterTarget = setMeterTo(voltMeterPin, fsds.getValueOf(voltMeterPin));
         }
    }
    
    if(payLoadString.startsWith("22402:")) {
         String valueString = payLoadString.substring(6, payLoadString.length());
         int dest = stringToInt(valueString);
               
         if (dest >= 0 && dest <= fsds.getValueOf(ampMeterPin)) {  // TODO maybe not in the correct place - setMeterTo?        
           ampMeterTarget = setMeterTo(ampMeterPin, dest);           
         } else {                       
           ampMeterTarget = setMeterTo(ampMeterPin, fsds.getValueOf(ampMeterPin));
         }
    }
    
    if(payLoadString.startsWith("ok")) {
       greenBrightness = 255;
       redBrightness = 0;
    }
    
    if(payLoadString.startsWith("problem")) {
       greenBrightness = 0;
       redBrightness = 255;
    }
    
    if(payLoadString.startsWith("count:")) {
         String valueString = payLoadString.substring(6, payLoadString.length());
         int dest = stringToInt(valueString);         
         countTarget = dest; 
    }  
    
}

// Calculate the correct PWM voltage required to move the a meter needle to the desired position
int setMeterTo(int pin, int dest) {
  String message = "Setting meter " + String(pin, DEC) + " to " + String(dest, DEC);
  publishString(message);
  
  double ratioOfFSD =(double) dest / fsds.getValueOf(pin);
  
  int zeroedPWMValue = zeroedPinouts.getValueOf(pin);
  int fsdPWMValue = fsdPinouts.getValueOf(pin);
  
  int offset = zeroedPWMValue + ((ratioOfFSD) * (fsdPWMValue - zeroedPWMValue));
  message = "PWM offset is: " + String(offset, DEC);
  publishString(message);
  return offset;
}


void panMeterFromTo(int pin, int offset) { 
    int currentPosition = positions.getValueOf(pin);
    if (currentPosition < offset && offset <= fsdPinouts.getValueOf(pin)) {
       //String message = "Panning meter " + String(pin, DEC) + " from " + String(currentPosition, DEC) + " to " + String(offset, DEC);
      //publishString(message);
       
      currentPosition = currentPosition + 1;
      analogWrite(pin, currentPosition);
      recordPosition(pin, currentPosition);
      return;
    }
   
    if (currentPosition > offset && offset >=  zeroedPinouts.getValueOf(pin)) {       
      //String message = "Panning meter " + String(pin, DEC) + " from " + String(currentPosition, DEC) + " to " + String(offset, DEC);
      //publishString(message);
      currentPosition = currentPosition - 1;     
      analogWrite(pin, currentPosition);
      recordPosition(pin, currentPosition);
      return;
   } 
}

void recordPosition(int pin, int position) {
   int index = positions.getIndexOf(pin);
   positionsRawArray[index].setValue(position);
}

int stringToInt(String value) {
    char numbers[100];
    value.toCharArray(numbers, value.length() + 1);
    int dest = atoi(numbers);
    return dest;
}

void publishString(String message) {
}

