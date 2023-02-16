var mqtt = require('mqtt')

var options = {
    host: '827677fef74a4840be0e4364cff5581f.s2.eu.hivemq.cloud',
    port: 8883,
    protocol: 'mqtts',
    username: 'mmarques291',
    password: 'm4t3us291s04r3s'
}

// initialize the MQTT client
var client = mqtt.connect(options);

// setup the callbacks
client.on('connect', function () {
    console.log('Connected');
});

client.on('error', function (error) {
    console.log(error);
});

client.on('message', function (topic, message) {
    // called each time a message is received
    console.log('Received message:', topic, message.toString());
});

// subscribe to topic 'my/test/topic'
client.subscribe('my/test/topic');

// publish message 'Hello' to topic 'my/test/topic'
client.publish('my/test/topic', 'Hello');