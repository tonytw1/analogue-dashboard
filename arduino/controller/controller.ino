#include <PubSubClient.h>

#include <SPI.h>
#include "Ethernet.h"
#include <HashMap.h>

// Ethernet config
#include <PubSubClient.h>

byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Pins which devices are attached to.
// The servo driver and Ethernet shield tend to disable PWM pins 9 and above.
// PWM doesn't seeem to be available on pin 4 ethier.
int ampMeterPin = 3;
int voltMeterPin = 5;
int greenPin = 9;
int redPin = 6;
int led = 13;

int greenBrightness = 0;
unsigned long greenNextStep = 0;
int ampMeterNextStep = 0;
unsigned long voltMeterNextStep = 0;

int redBrightness = 0;
unsigned long redNextStep = 0;
int ampMeterTarget = 0;
int voltMeterTarget = 0;

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

int panDelay = 100;

EthernetClient ethClient;
byte server[] = { 192, 168, 1, 10 };
PubSubClient client(server, 1883, callback, ethClient);

void setup() {
   pinMode(led, OUTPUT);
   digitalWrite(led, LOW);

   Serial.begin(9600);
   
   // start the Ethernet connection:
   if (Ethernet.begin(mac) == 0) {
      // failed to dhcp no point in carrying on, so do nothing forevermore:
      for(;;)
      ;
  }
  
  digitalWrite(led, HIGH);
  
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
  }
  
  // PWM voltage corresponding to zero and FSD for meters
  zeroedPinouts[0](ampMeterPin, 0);
  zeroedPinouts[1](voltMeterPin, 0);
  
  fsdPinouts[0](ampMeterPin, 250);
  fsdPinouts[1](voltMeterPin, 253);
  
  // The full scale deflection value on the face of the amp meter
  fsds[0](ampMeterPin, 100);
  fsds[1](voltMeterPin, 80);
  
  // All devices are initially zeroed  TODO - need to be able to define rest locations for all - ie. gauges should reset in center - 0 could be of scale 
  positions[0](ampMeterPin, 0);
  positions[1](voltMeterPin, 0);
  
  // Power up pins
  pinMode(ampMeterPin, OUTPUT);
  pinMode(voltMeterPin, OUTPUT);  
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  
  // Onboard LED pin used to signal activity only
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  
  digitalWrite(greenPin, HIGH);
  delay(1000);
  digitalWrite(greenPin, LOW);
   
  digitalWrite(redPin, HIGH);
  delay(1000);
  digitalWrite(redPin, LOW);
  
  //setMeterTo(ampMeterPin, 10);
  //setMeterTo(voltMeterPin, 10);
  //delay(2000);
 
  setMeterTo(ampMeterPin, 0);
  setMeterTo(voltMeterPin, 0);
  //delay(2000);
      
  if (client.connect("zabbix")) {
      client.publish("gauges","arduino connected");
      client.subscribe("zabbix");
      
     digitalWrite(greenPin, HIGH);
     digitalWrite(redPin, HIGH);
     delay(1000);

     digitalWrite(redPin, LOW);
     digitalWrite(greenPin, LOW); 
     
  } else {
  }
  
}

void loop()  {
   client.loop();
    
   if (millis() > greenNextStep) {
      greenNextStep = millis() + 10;       
      if (greenBrightness > 30 ) {
         greenBrightness = greenBrightness -1;
      }
      analogWrite(greenPin, greenBrightness);
   }
   
   if (millis() > redNextStep) {
     redNextStep = millis() + 10;       
     if (redBrightness > 30 ) {
        redBrightness = redBrightness -1;
     }
     analogWrite(redPin, redBrightness);
   }
   
   if (millis() > voltMeterNextStep) {
     voltMeterNextStep = millis() + 100;       
     panMeterFromTo(voltMeterPin, voltMeterTarget);
   }
   
   digitalWrite(13, HIGH);  
   digitalWrite(13, LOW);

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
