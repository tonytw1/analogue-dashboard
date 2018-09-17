# Arduino sketches

Controls the Arduino board which drives the lamps and gauges.

Receives metric values over the USB serial port.
Metrics are expected to be in this format:

```
[Lamp name]:[value]
```

ie.
```
amps:23
```

The board periodally announces the available lamps and gauges over the serial interface in this format:
```
[Lamp name]:[FSD]
```

ie.
```
volts:80
amps:100
greenlamp:1
redlamp:1
```
