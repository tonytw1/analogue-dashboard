#include <bluefruit.h>

unsigned int count = 0;
unsigned long last_pulse_time = 0;
unsigned long last_pulse_width = 0;

void setup() 
{
  Bluefruit.begin();

  // off Blue LED for lowest power consumption
  Bluefruit.autoConnLed(false);
  
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(0);

  attachInterrupt(A0, incrementCount, RISING);
  
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

bool setAdvertisingData(BLEAdvertising& adv, unsigned int count)
{  
  struct ATTR_PACKED
  {
    uint16_t count;
    unsigned long last_pulse_width;
  } beacon_data =
  {
    count = count,
    last_pulse_width = last_pulse_width
  };

  VERIFY_STATIC(sizeof(beacon_data) == 6);
  adv.clearData();
  adv.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  return adv.addData(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, &beacon_data, sizeof(beacon_data));
}

void startAdv(void)
{
  setAdvertisingData(Bluefruit.Advertising, count);
  
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

void loop() 
{
  delay(1000);                   // 0 = Don't stop advertising after n seconds  
  Bluefruit.Advertising.stop();
  setAdvertisingData(Bluefruit.Advertising, count);
  Bluefruit.Advertising.start(0);
}
