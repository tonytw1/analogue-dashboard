#!/usr/bin/python
from bluepy.btle import Scanner, DefaultDelegate
import paho.mqtt.client as mqtt
import re

mqtt_topic = 'metrics'

mqtt_client = mqtt.Client()
mqtt_client.connect("10.0.45.11", 32183, 60)

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
	print "Received new data from", dev.addr, dev.getScanData(), dev.rssi, dev.addrType, dev.getValueText(dev.addrType), dev.updateCount
	short_addr = re.sub(':', '', dev.addr)
	message = short_addr + ":" + str(dev.rssi)
	mqtt_client.publish(mqtt_topic, message)

scanner = Scanner().withDelegate(ScanDelegate())
scanner.start(passive=True)
while True:
    print "Still running..."
    scanner.process()

