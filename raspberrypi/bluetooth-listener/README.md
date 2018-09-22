## Bluetooth listener

Passively listens for Bluetooth LE advertisements and echos them onto MQTT.

Provides a way for the mertics system to pick up sensor data (such as power consumption) wirelessly from hard to reach locations.


Additional permissions required to run as non root.

```
sudo setcap 'cap_net_raw,cap_net_admin+eip' /usr/local/lib/python2.7/dist-packages/bluepy/bluepy-helper
```
