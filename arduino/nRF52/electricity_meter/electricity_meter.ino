#include <bluefruit.h>

unsigned int count = 0;
unsigned long last_pulse_time = 0;
unsigned long last_pulse_width = 0;
unsigned int battery = 0;
int battery_interval = 0;

#define VBAT_PIN          (A7)
#define VBAT_DIVIDER_COMP (1.403F)        // Compensation factor for the VBAT divider
#define VBAT_MV_PER_LSB   (0.73242188F)   // 3.0V ADC range and 12-bit ADC resolution = 3000mV/4096

void setup() 
{
  Bluefruit.begin();

  // off Blue LED for lowest power consumption
  Bluefruit.autoConnLed(false);
  
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(0);

  attachInterrupt(A0, incrementCount, RISING);
  battery = readVBAT();
  
  // Setup the advertising packet
  startAdv();
}

void incrementCount() {
  count = (unsigned int) (count +  1);
  
  unsigned long previous_pulse_time = last_pulse_time;
  last_pulse_time = millis();
  if (previous_pulse_time != 0 && last_pulse_time > previous_pulse_time) {
    last_pulse_width = last_pulse_time - previous_pulse_time;
  }
  
}

bool setAdvertisingData(BLEAdvertising& adv, unsigned int count, unsigned int battery)
{  
  struct ATTR_PACKED
  {
    uint16_t count;
    unsigned long last_pulse_width;
    uint16_t battery;
  } beacon_data =
  {
    count = count,
    last_pulse_width = last_pulse_width,
    battery = battery
  };

  VERIFY_STATIC(sizeof(beacon_data) == 8);
  adv.clearData();
  adv.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  return adv.addData(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, &beacon_data, sizeof(beacon_data));
}

void startAdv(void)
{
  setAdvertisingData(Bluefruit.Advertising, count, battery);
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * Apple Beacon specs
   * - Type: Non connectable, undirected
   * - Fixed interval: 100 ms -> fast = slow = 100 ms
   */
  //Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_ADV_NONCONN_IND);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(1600, 1600);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

// From Adafruit examples
unsigned int readVBAT() {
  // Set the analog reference to 3.0V (default = 3.6V)
  analogReference(AR_INTERNAL_3_0);

  // Set the resolution to 12-bit (0..4095)
  analogReadResolution(12); // Can be 8, 10, 12 or 14

  // Let the ADC settle
  delay(1);

  // Get the raw 12-bit, 0..3000mV ADC value
  int vbat_raw = analogRead(VBAT_PIN);

  // Set the ADC back to the default settings
  analogReference(AR_DEFAULT);
  analogReadResolution(10);
  
  // Convert the raw value to compensated mv, taking the resistor-
  // divider into account (providing the actual LIPO voltage)
  // ADC range is 0..3000mV and resolution is 12-bit (0..4095),
  // VBAT voltage divider is 2M + 0.806M, which needs to be added back
  float vbat_mv = (float) vbat_raw * VBAT_MV_PER_LSB * VBAT_DIVIDER_COMP;

  return (unsigned int) round(vbat_mv);
}

void loop() 
{
  delay(1000);                   // 0 = Don't stop advertising after n seconds

  battery_interval = battery_interval + 1;
  if (battery_interval > 600) {
    battery = readVBAT();
    battery_interval = 0;
  }
  
  Bluefruit.Advertising.stop();
  setAdvertisingData(Bluefruit.Advertising, count, battery);
  Bluefruit.Advertising.start(0);
}
