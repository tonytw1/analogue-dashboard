unsigned int announcementInterval = 60000;
unsigned long updateTTL = 90000;

unsigned long nextAnnouncement = 0;

String inputString = "";     // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

unsigned int lampPins[] = {8, 9, 10, 11, 6, 7, 0, 5, 4};
String lampNames[] = {"green", "red", "red2", "green2", "amps", "volts", "counter", "linear1", "linear2"};
unsigned long lampNextPan[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int lampPanSpeeds[] = {5, 5, 5, 5, 100, 100, 3, 50, 50};
unsigned int lampFSDs[] = {1, 1, 1, 1, 100, 80, 9999, 10, 10};

int lampTargets[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int lampValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
long lampExpiry[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

int counterDataPin = 53;
int counterLatchPin = 52;
int counterClockPin = 51;

int counterMultiplexSegment = 0;

void setup() {
  inputString.reserve(200);
  Serial.begin(115200);

  pinMode(counterDataPin, OUTPUT);
  pinMode(counterLatchPin, OUTPUT);    
  pinMode(counterClockPin, OUTPUT); 
    
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  int i;
  int numberOfElements = sizeof(lampPins) / sizeof(lampPins[0]);
  for (i = 0; i < numberOfElements; i = i + 1) {
    int lampPin = lampPins[i];
    pinMode(lampPin, OUTPUT);
    digitalWrite(lampPin, LOW);
  }
}

void loop() { 
  pan();
  expireStaleLamps();
  if (millis() > nextAnnouncement) {
    announce();
  }   
}

void announce() {
  int i;
  int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
  for (i = 0; i < numberOfElements; i = i + 1) {
    Serial.println("lamp:" + lampNames[i] + "[" + lampFSDs[i] + "]");
  }
    
  nextAnnouncement = millis() + announcementInterval;
}

void pan() {
  int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
  for (int i = 0; i < numberOfElements; i = i + 1) {
    int current = lampValues[i];
    
    if (lampNextPan[i] < millis()) {
      int value = lampValues[i];
      int target = lampTargets[i];
      
      int next = value;
      if (target < value) {       
        next = next - 1;           
      } else if (target > value) {
        next = next + 1;              
      }
      lampValues[i] = next;
      lampNextPan[i] = millis() + lampPanSpeeds[i];
    }
    
    String lampName = lampNames[i]; 
    int next = lampValues[i];
    
    if (lampName == "counter") {
       refreshCounter(counterMultiplexSegment, next, counterDataPin, counterLatchPin, counterClockPin);
      counterMultiplexSegment =counterMultiplexSegment + 1;
       if (counterMultiplexSegment > 3) {  // TODO remove hard coded length
         counterMultiplexSegment = 0;
       }
                    
    } else {
      if (next != current) {
        int lampPin = lampPins[i];     
        analogWrite(lampPin, next);      
      }
    }   
  }
  
}

void expireStaleLamps() {
  int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
  for (int i = 0; i < numberOfElements; i = i + 1) {
    long expiry = lampExpiry[i];
    if (expiry < millis()) {
      lampTargets[i] = 0;
    }
  }
}

// Write out a digit to a 7595 latched 7 segment display group and select the multiplexed module
void refreshCounter(int module, int c, int dataPin, int latchPin, int clockPin) {
   int scale = round(pow(10, module));
   int num = c / scale % 10;

   // Building the 8 bit byte we are going to latch into the 595.
   // This byte will contain 4 bits of binary number and 4 bits to select the active display segment
   // (the 4 segments are multiplexed - only 1 is actually lit at anyone time).
   // ie. for the digit 3:
   // [00110000]
      
   // Left shift the first 4 bits of the digit into the byte.
   byte data = num << 4; 

   // We'd like to turn off the leading zeros on the display to improve readability.
   boolean leadingBlank = num == 0 & scale > 1 && (scale > c);           
   if (!leadingBlank) {
     // The individual 7 segment display modules are selected by the 4 right most bits of the 595.
     // To display the current digit we set the bit for this module.
     // ie. to display a 3 on the 2nd module
     // [00110010]
     bitSet(data, module);
   }

   // Send the byte the 595 and latch it
   digitalWrite(latchPin, LOW);
   shiftOut(dataPin, clockPin, LSBFIRST, data);
   digitalWrite(latchPin, HIGH);
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char) Serial.read();   
    if (inChar == '\n') {
       stringComplete = true;
    } else {
      inputString += inChar;     
    }

    if (stringComplete) {
      processInput(inputString);
      inputString = "";
      stringComplete = false;
    }
  }
}

void processInput(String input) {
  if (input.length() > 0 ) {
    int i;
    int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
    for (i = 0; i < numberOfElements; i = i + 1) {
      String lampName = lampNames[i];
      String prefix = lampName + ":";

      if (input.startsWith(prefix)) {
        String value = input.substring(prefix.length());
        double valueAsDouble = stringToDouble(value);
        if (valueAsDouble < 0) {
          valueAsDouble = 0;
        }

        if (lampName == "counter") {
          lampTargets[i] = (int) valueAsDouble;

        } else {
          // For meters set the target to the desired PWM value
          double ratioOfFSD = valueAsDouble / lampFSDs[i];
          int zeroedPWMValue = 0;
          int fsdPWMValue = 255;
          int offset = (int) zeroedPWMValue + ((ratioOfFSD) * (fsdPWMValue - zeroedPWMValue));
          lampTargets[i] = offset;               
        }
        
        lampExpiry[i] = millis() + updateTTL;
      }      
    }
  }
}

double stringToDouble(String value) {
    return value.toDouble();
}

