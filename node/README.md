# MQTT to serial bridge

Uses Node to provide a bidirectional MQTT to serial bridge.
Routes incoming MQTT messages to the Arduino attached to the Raspberry Pi's USB port.


Install node and node package manager.
```
sudo apt-get install nodejs
sudo apt-get install npm
sudo npm install -g npm@next
```

Install packages
```
npm install serialport --unsafe-perm --build-from-source
npm install mqtt
```

```
node serial-mqtt-bridge.js
```

TODO how to run as a service?
