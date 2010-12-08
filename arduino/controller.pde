#include <Servo.h>
#include <SPI.h>
#include "Ethernet.h"
#include <WebServer.h>

// Ethernet config
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static uint8_t ip[] = { 192, 168, 1, 30 };
WebServer webserver("", 80);

// Pins which devices are attached to.
// The servo driver and Ethernet shield tend to disable PWM pins 9 and above
int ampMeterPin = 3;
int gaugePin = 2;


// The full scale deflection value on the face of the amp meter
int ampMeterFSD = 500;

// PWM voltage corresponding to zero and FSD on the amp meter
int min = 25 ;   
int max = 251;
int current = 0;
int panDelay = 100;

// Servo offsets for zero and FSD for the pressure gauge
int gaugeMin = 79;
int gaugeCenter = 105;
int gaugeMax = 136;

Servo servo;

#define NAMELEN 32
#define VALUELEN 32

void mainCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
  server.httpSuccess();
  
  if (type != WebServer::HEAD) {
    
    URLPARAM_RESULT rc;
    bool repeat;
    char name[NAMELEN], value[VALUELEN];
   
    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (!(rc == URLPARAM_EOS)) {
        if (strcmp(name, "gauge") == 0) {         
          int dest = stringToInt(value);                 
          if (dest >= gaugeMin && dest <= gaugeMax) {
             server.print(name);
             server.print(":");
             server.print(dest);
             moveGaugeTo(dest);
          }
        }
        
        if (strcmp(name, "amps") == 0) {         
          int dest = stringToInt(value);                 
          if (dest >= 0 && dest <= fsd) {
             server.print(name);
             server.print(":");
             server.print(dest);
             server.print(setAmpMeterTo(dest));
          }
        }
                
      }
    }
  }
}


void setup() {  
  Ethernet.begin(mac, ip);
  webserver.addCommand("index.html", &mainCmd);
  webserver.begin();
  
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  
  pinMode(ampMeterPin, OUTPUT);
  setAmpMeterTo(0);
  
  servo.attach(gaugePin, min, max);
  moveGaugeTo(gaugeCenter);
}


void loop()  {
  webserver.processConnection();  
  delay(10);
}


// Calculate the correct PWM voltage required to move the amp meter needle to the desired position
int setAmpMeterTo(int dest) {
  double ratioOfFSD =(double) dest / ampMeterFSD;  
  int offset = min + ((ratioOfFSD) * (max - min));  
  if (offset >= min && offset <= max) {
    panAmpMeterFromTo(current, offset);  
  }
  return offset;
}


// Move the needle in an orderly fashion to avoid recoil.
// The needle has a fair amount of momentum and we don't want to damage the spring with sudden stops.
// Adding alot of capacitance across the meter would also be a good thing todo (say > 200mF)
void panAmpMeterFromTo(int start, int offset) { 
    int position = start;
    while (position < offset && offset <= max) {
       position = position + 1;
       Serial.print("Current: ");
       Serial.println(position, DEC);
       analogWrite(ampMeterPin, position);
       delay(panDelay);
     }
   
     while (position > offset && offset >= min) {
      position = position - 1;
      Serial.print("Current: ");
      Serial.println(position, DEC);
      analogWrite(ampMeterPin, position);
      delay(panDelay);
   }
   current = position;
   return;
}


// Slowly walk the gauge needle to the desired position to avoid stressing the mechanism with snappy movements.
void moveGaugeTo(int dest) {
   int currentPos = servo.read(); 
   while (currentPos < dest && dest <= gaugeMax) {
       currentPos = currentPos + 1;
       servo.write(currentPos);
       delay(panDelay);
   }
   
   while (currentPos > dest && dest >= gaugeMin) {
      currentPos = currentPos - 1;
      servo.write(currentPos);
      delay(panDelay);
   }
}


int stringToInt(String value) {
    char numbers[100];
    value.toCharArray(numbers, value.length() + 1);
    int dest = atoi(numbers);
    return dest;
}