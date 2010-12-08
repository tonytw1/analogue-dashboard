#include <Servo.h>
#include <SPI.h>
#include "Ethernet.h"
#include <WebServer.h>

static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static uint8_t ip[] = { 192, 168, 1, 30 };
WebServer webserver("", 80);

String ampsKey ="AMPS:";
String gaugeKey ="GAUGE:";

int ampMeterPin = 3;
int gaugePin = 2;

// The full scale deflection value for this gauge
int fsd = 500;

// PWM voltages for 0 to FSD
int min = 25 ;   
int max = 251;
int current = 0;
int panDelay = 100;

int gaugeMin = 79;
int gaugeCenter = 105;
int gaugeMax = 136;

Servo servo;

#define NAMELEN 32
#define VALUELEN 32


void gaugeCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
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
  Serial.begin(9600);
  
  Ethernet.begin(mac, ip);
  webserver.addCommand("index.html", &gaugeCmd);
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
  
  String line = readLine();
  Serial.flush();
  
  if (line.startsWith(ampsKey) == true) {
    String value = value.concat(line.substring(ampsKey.length()));
    int dest = stringToInt(value);        
    if (dest >= 0 && dest <= fsd) {
       setAmpMeterTo(dest);
    }
  }
  
  if (line.startsWith(gaugeKey) == true) {
      String value = value.concat(line.substring(gaugeKey.length()));
      int dest = stringToInt(value);                 
      if (dest >= gaugeMin && dest <= gaugeMax) {
        moveGaugeTo(dest);
      }
  }
  
  delay(10);
}



int stringToInt(String value) {
    char numbers[100];
    value.toCharArray(numbers, value.length() + 1);
    int dest = atoi(numbers);
    return dest;
}


int setAmpMeterTo(int dest) {
  Serial.print("Requested setting: ");
  Serial.println(dest);
  
  double ratio =(double) dest / 500;
  Serial.print("Ratio of FSD: ");
  Serial.println(ratio);
  
  int offset = min + ((ratio) * (max - min));
  
  if (offset >= min && offset <= max) {
    panAmpMeterFromTo(current, offset);  
  } 
 
  return offset;
}


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




void moveGaugeTo(int dest) {
    Serial.print("Moving gauge to: ");
    Serial.println(dest);
    
   digitalWrite(13, HIGH);
   int currentPos = servo.read(); 
   while (currentPos < dest && dest <= gaugeMax) {
       currentPos = currentPos + 1;
       Serial.print("Current: ");
       Serial.println(currentPos, DEC);
       servo.write(currentPos);
       delay(panDelay);
   }
   
   while (currentPos > dest && dest >= gaugeMin) {
      currentPos = currentPos - 1;
      Serial.print("Current: ");
      Serial.println(currentPos, DEC);
      servo.write(currentPos);
      delay(panDelay);
   }
   digitalWrite(13, LOW);   
}


String readLine() {
  String line;
  while(Serial.available()) {  // buffer up a line
    char c;
    c = Serial.read();
    line = line + c;
  }
  line = line.trim();
  Serial.flush();
  return line;
}