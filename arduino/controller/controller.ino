#include <PubSubClient.h>

#include <SPI.h>
#include "Ethernet.h"
#include <HashMap.h>

// Ethernet config
#include <PubSubClient.h>

byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Pins which devices are attached to.
// The servo driver and Ethernet shield tend to disable PWM pins 9 and above.
// PWM doesn't seem to be available on pin 4 ethier.

int redPin = 9;
int greenPin = 3;
int voltMeterPin = 0;
int ampMeterPin = 0;  // TODO

int greenBrightness = 0;
unsigned long greenNextStep = 0;
int ampMeterNextStep = 0;
unsigned long voltMeterNextStep = 0;
unsigned long countNextStep = 0;

int redBrightness = 0;
unsigned long redNextStep = 0;
int ampMeterTarget = 0;
int voltMeterTarget = 0;
int count = 0;
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

EthernetClient ethClient;
byte server[] = { 192, 168, 1, 10 };
PubSubClient client(server, 1883, callback, ethClient);

void setup() {
   Serial.begin(9600);
   
    pinMode(greenPin, OUTPUT);    
    pinMode(redPin, OUTPUT);     
    
    pinMode(5, OUTPUT);    
    pinMode(4, OUTPUT);     
    pinMode(3, OUTPUT);     
    
    pinMode(9, OUTPUT);    
    pinMode(8, OUTPUT);     
    pinMode(7, OUTPUT);     
    pinMode(6, OUTPUT);
    
   // start the Ethernet connection:
   if (Ethernet.begin(mac) == 0) {
      // failed to dhcp no point in carrying on, so do nothing forevermore:
      for(;;)
      ;
  }
    
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
   }
  
  // PWM voltage corresponding to zero and FSD for meters
  zeroedPinouts[0](ampMeterPin, 0);
  zeroedPinouts[1](voltMeterPin, 0);
  zeroedPinouts[2](greenPin, 0);
  zeroedPinouts[3](redPin, 0);

  fsdPinouts[0](ampMeterPin, 250);
  fsdPinouts[1](voltMeterPin, 253);
  fsdPinouts[2](greenPin, 255);
  fsdPinouts[3](redPin, 255);

  // The full scale deflection value on the face of the amp meter
  fsds[0](ampMeterPin, 100);
  fsds[1](voltMeterPin, 80);
  fsds[2](greenPin, 255);
  fsds[3](redPin, 255);
  
  // All devices are initially zeroed  TODO - need to be able to define rest locations for all - ie. gauges should reset in center - 0 could be of scale 
  positions[0](ampMeterPin, 0);
  positions[1](voltMeterPin, 0);
  positions[2](greenPin, 0);
  positions[3](redPin, 0);

  // Power up pins
  pinMode(ampMeterPin, OUTPUT);
  pinMode(voltMeterPin, OUTPUT);  
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
    
  digitalWrite(greenPin, HIGH);
  delay(1000);
  digitalWrite(greenPin, LOW);
   
  digitalWrite(redPin, HIGH);
  delay(1000);
  digitalWrite(redPin, LOW);
  
 
  if (client.connect("zabbix")) {
      client.publish("gauges","arduino connected");
      client.subscribe("zabbix");
      
     digitalWrite(greenPin, HIGH);
     digitalWrite(redPin, HIGH);
     delay(1000);

     digitalWrite(redPin, LOW);
     digitalWrite(greenPin, LOW);    
  }
  
  setMeterTo(ampMeterPin, 0);
  setMeterTo(voltMeterPin, 0);
  setMeterTo(greenPin, 0);
  setMeterTo(redPin, 0);  
}

void loop()  {
   client.loop();
    
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
        
        digitalWrite(5, LOW);
        shiftOut(7, 6, LSBFIRST, digit);
        digitalWrite(5, HIGH);
        delay(1);         
    }
    
}

void callback(char* topic, byte* payload, unsigned int length) {
   //check for paylaod; is it there at all ?
  if (length > 0) {    
    
    // create character buffer with ending null terminator (string)
    int i = 0;
    char message_buff[100];
    for(i=0; i<length; i++) {
      message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
  
    String payLoadString = String(message_buff);
    
    if(payLoadString.startsWith("10047")) {
         String valueString = payLoadString.substring(6, payLoadString.length());
         int dest = stringToInt(valueString);
         
         String message = String("Requested to move volt meter to: ") + String(dest);
         publishString(message);
         
         if (dest >= 0 && dest <= fsds.getValueOf(voltMeterPin)) {  // TODO maybe not in the correct place - setMeterTo?
           String message = String("Moving volt meter to: ") + String(dest);
           publishString(message);           
           voltMeterTarget = setMeterTo(voltMeterPin, dest);
           
         } else {
            String message = String("Out of range: ") + String(dest);
            publishString(message);                           
            voltMeterTarget = setMeterTo(voltMeterPin, fsds.getValueOf(voltMeterPin));
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
    
    if(payLoadString.startsWith("count")) {
         String valueString = payLoadString.substring(6, payLoadString.length());
         int dest = stringToInt(valueString);         
         countTarget = dest; 
    }
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
    char charBuf[100];
    message.toCharArray(charBuf, 100);
    client.publish("gauges", charBuf);
}
