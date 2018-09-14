#!/usr/bin/python

# TODO Using passive scans only
from bluepy.btle import Scanner, DefaultDelegate

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
	print "Received new data from", dev.addr, dev.getScanData(), dev.rssi, dev.addrType, dev.getValueText(dev.addrType), dev.updateCount

scanner = Scanner().withDelegate(ScanDelegate())
scanner.start()
while True:
    print "Still running..."
    scanner.process()
