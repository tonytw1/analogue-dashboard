unsigned int announcementInterval = 10000;
unsigned long updateTTL = 90000;

unsigned long nextAnnouncement = 0;

String inputString = "";     // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

unsigned int lampPins[] = {8, 9, 10, 11, 6, 7};
String lampNames[] = {"green", "red", "green2", "red2", "amps", "volts", "counter"};
unsigned long lampNextPan[] = {0, 0, 0, 0, 0, 0, 0};
unsigned int lampPanSpeeds[] = {5, 5, 5, 5, 100, 100, 10};
unsigned int lampFSDs[] = {1, 1, 1, 1, 100, 80, 9999};

int lampTargets[] = {0, 0, 0, 0, 0, 0, 0};
int lampValues[] = {0, 0, 0, 0, 0, 0, 0};
long lampLastUpdated[] = {0, 0, 0, 0, 0, 0, 0};


int counterDataPin = 53;
int counterLatchPin = 52;
int counterClockPin = 51;

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
  if (stringComplete) {
    processInput();
  }

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
  int i;
  int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
  for (i = 0; i < numberOfElements; i = i + 1) {
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
      refreshCounter(next, counterDataPin, counterLatchPin, counterClockPin);        

    } else {      
      double ratioOfFSD =(double) next / lampFSDs[i];  
      int zeroedPWMValue = 0;
      int fsdPWMValue = 255;
      int offset = zeroedPWMValue + ((ratioOfFSD) * (fsdPWMValue - zeroedPWMValue));

      int lampPin = lampPins[i];     
      analogWrite(lampPin, offset);
    }   
  }
  
}

void processInput() {
  if (inputString.length() > 0 ) {
    int i;
    int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
    for (i = 0; i < numberOfElements; i = i + 1) {
      String lampName = lampNames[i];
      String prefix = lampName + ":";

      if (inputString.startsWith(prefix)) {
        String value = inputString.substring(prefix.length());
        int valueAsInt = stringToInt(value);     
        lampTargets[i] = valueAsInt;         
        lampLastUpdated[i] = millis() + updateTTL;
      }
    }
 }

 inputString = "";
 stringComplete = false;
}

void expireStaleLamps() {
  int i;
  int numberOfElements = sizeof(lampNames) / sizeof(lampNames[0]);
  for (i = 0; i < numberOfElements; i = i + 1) {
      long lastUpdated = lampLastUpdated[i];
      if (lastUpdated < millis()) {
        lampTargets[i] = 0;
      }
  }  
}

// Write out a number to a 7595 latched 7 segment display group
void refreshCounter(int c, int dataPin, int latchPin, int clockPin) {
 boolean leadingBlank = true;           
 for (int i = 3; i >= 0; i--) {     
    int num = c%10;
     if (i == 1) {
       num = (c/10%10);
     }
     if (i == 2) {
       num = (c/100%10);
     }
     if (i == 3) {
       num = (c/1000%10);
     }
      
     if (num > 0 || i == 0) {
       leadingBlank = false; 
     }
              
     byte digit = num << 4;
     if (!leadingBlank) {
       bitSet(digit, i);  // TODO document the encoding of digit; looks like half of the digit is been used as address lines
     }

     digitalWrite(latchPin, LOW);
     shiftOut(dataPin, clockPin, LSBFIRST, digit);
     digitalWrite(latchPin, HIGH);
  }

}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char) Serial.read();
    // add it to the inputString:
    if (inChar != '\n') {
      inputString += inChar;
    }
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

int stringToInt(String value) {
    char numbers[100];
    value.toCharArray(numbers, value.length() + 1);
    int dest = atoi(numbers);
    return dest;
}

