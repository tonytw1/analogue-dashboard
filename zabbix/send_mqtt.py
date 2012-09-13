import os
from zabbix_api import ZabbixAPI

server="http://localhost/zabbix"
username="admin"
password="zabbix"

zapi = ZabbixAPI(server=server, path="", log_level=0)
zapi.login(username, password)

key='apache[localhost,TotalAccesses]'

items=zapi.item.get({"select_hosts":"extend","output":"extend","filter":{"key_":key}})
for item in items:
	if item['lastvalue'] is not None:
		cmd = 'mosquitto_pub -h localhost -t zabbix -m {0}:{1}'.format(item['hostid'], item['lastvalue'])
		os.system(cmd)

triggers=zapi.trigger.get({"monitored":"true", "output":"extend", "min_severity":"4", "only_true":"true"});
problem = False;
for trigger in triggers:
	if trigger['value'] == "1":
		problem = True;

if problem:
        cmd = 'mosquitto_pub -h localhost -t zabbix -m problem';
        os.system(cmd)
else:
        cmd = 'mosquitto_pub -h localhost -t zabbix -m ok';
        os.system(cmd)
