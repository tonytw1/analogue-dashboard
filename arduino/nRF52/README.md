## Adafruit Feather nRF52 Bluefruit LE

Bluetooth enabled board. Used to capture pulse counts from the electrity meter.
Transmits the accumulated pulse count (which gives total consumption) and the time interval between the last 2 pulses (which infers current rate of consumption).

The electicity meter has an LED indicator which pulses 800 times per kilowatt hour. A sensor is attached in front of the LED.
A pulse raises an interrupt which increments the pulse count.

The main loop periodically wakes and updates the contents of the Bluetooth broadcast.

Payload from left to right:
2 byte integer pulse count
4 byte time between last pair of pulses in milliseconds

````
3700591c0000
````

ie. 37 pulses; 7,229ms between the last 2 pulses.
