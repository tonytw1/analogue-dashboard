#!/usr/bin/python
from bluepy.btle import Scanner, DefaultDelegate, ScanEntry
import paho.mqtt.client as mqtt
import re
import binascii
import struct

devices = ['f2:67:bf:fc:aa:98']
mqtt_topic = 'metrics'

mqtt_client = mqtt.Client()
mqtt_client.connect("10.0.45.11", 32183, 60)

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
	print "Received new data from", dev.addr, dev.getScanData(), dev.rssi, dev.addrType, dev.getValueText(dev.addrType), dev.updateCount
	if (dev.addr in devices):
		short_addr = re.sub(':', '', dev.addr)
		data_items = dev.getScanData()
		manufacturer_specific_data_item = filter(lambda x: x[0] == ScanEntry.MANUFACTURER, data_items)
		if (len(manufacturer_specific_data_item) > 0):
			value = manufacturer_specific_data_item[0][2]
			count_hex = value[0:4]
			count_bytes = binascii.unhexlify(count_hex)
			count = struct.unpack('H', count_bytes)[0]

			pulse_duration_hex = value[4:12]
			pulse_duration_bytes = binascii.unhexlify(pulse_duration_hex)
			pulse_duration = struct.unpack('L', pulse_duration_bytes)[0]

			count_message = short_addr + "count:" + str(count)
			mqtt_client.publish(mqtt_topic, count_message)

			pulse_duration_message = short_addr + "pulseduration:" + str(pulse_duration)
			mqtt_client.publish(mqtt_topic, pulse_duration_message)

scanner = Scanner().withDelegate(ScanDelegate())
scanner.start(passive=True)
while True:
    print "Still running..."
    scanner.process()
