var SerialPort = require('serialport');
var bindPhysical = require('mqtt-serial').bindPhysical;
var mqtt = require('mqtt');
 
// setup the mqtt client with port, host, and optional credentials 
var client = mqtt.connect('mqtt://127.0.0.1:1883');

// setup a connection to a physical serial port 
var serialPort = new SerialPort('/dev/ttyACM0',{
    baudRate: 115200
});


// Forked from https://github.com/monteslu/mqtt-serial/blob/master/index.js - subscribe to readline parser
function bindSerialToMqtt(options){
  var client = options.client;
  var serialPort = options.serialPort;
  var receiveTopic = options.receiveTopic;
  var transmitTopic = options.transmitTopic;
  var qos = options.qos || 0;
   
  var parser = serialPort.pipe(new SerialPort.parsers.Readline);

  function serialWrite(data){
    try{
      console.log('Writing to serial port: ', data.toString());
      serialPort.write(data);
    }catch(exp){
      console.log('error reading message', exp);
    }
  }

  client.subscribe(receiveTopic, {qos: qos});

  parser.on('data', function(data){	// Important; subscribing to parser.on gives you full lines; serialPort.on gives you incomplete chunks
    console.log("Publishing serial line to MQTT: " + data.toString());
    client.publish(transmitTopic, data, {qos: qos});
  });

  client.on('message', function(topic, data, packet){
    try{
      if(topic === receiveTopic){
        serialWrite(data);
      }
    }catch(exp){
      console.log('error on message', exp);
      //self.emit('error', 'error receiving message: ' + exp);
    }
  });
}

//connects the physical device to an mqtt server 
bindSerialToMqtt({
  serialPort: serialPort,
  client: client,
  transmitTopic: 'gauges',
  receiveTopic: 'gauges'
});
