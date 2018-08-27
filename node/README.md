# MQTT to serial bridge

Uses Node to provide a bidirectional MQTT to serial bridge.
Routes incoming MQTT messages to the Arduino attached to the Raspberry Pi's USB port.

Node with the serialport and mqtt modules gave the least resistance.


## Install

Install node and node package manager.
```
sudo apt-get install nodejs
sudo apt-get install npm
```

npm will be too old to install latest modules so bootstrap it up to the latest release outside of the apt-get.

```
sudo npm install -g npm@next
```

Install packages locally
```
mkdir /opt/node
cd /opt/node
npm install serialport --unsafe-perm --build-from-source
npm install mqtt
```

Check
```
node serial-mqtt-bridge.js
```


## Run as a systemd service

```
cp serial-mqtt-bridge.service /lib/systemd/system/serial-mqtt-bridge.service
sudo systemctl daemon-reload
sudo service my-test-service start
sudo systemctl enable serial-mqtt-bridge
```
