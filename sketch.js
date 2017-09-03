// To setup UART device
var NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
var NUS_TX_CHARACTERISTIC_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
var NUS_RX_CHARACTERISTIC_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";

var bluetoothDevice = null;
var txCharacteristic = null;
var rxCharacteristic = null;

// Sensor Reading Communication Buffer
var currentReading = new String();
var notificationBuffer = new String();

// Sensor Readings
var pitch = null;
var roll = null;
var acceleration = null;

var debug=false;


function setup() {
  createCanvas(640, 480,WEBGL);
  background('#333333');

}

function draw() {
  background('#333333');


  if(pitch != null && roll != null) {

    push();

  	//Rotate based on pitch and roll
  	rotateX(pitch*PI/180.0);
  	rotateZ(roll*PI/180.0);
    if(debug) print(pitch,roll);

    // Draw a scaled cube
  	scale(90);
  	beginShape();


    strokeWeight(5);

  	fill(0, 255, 0); vertex( 1,  1,  1);
  	fill(0, 255, 0); vertex( 1, -1,  1);
  	fill(0, 255, 0); vertex(-1, -1,  1);

  	fill(0, 255, 255); vertex( 1,  1,  1);
  	fill(0, 255, 255); vertex( 1,  1, -1);
  	fill(0, 255, 255); vertex( 1, -1, -1);
  	fill(0, 255, 255); vertex( 1, -1,  1);


  	fill(255, 0, 255); vertex( 1,  1, -1);
  	fill(255, 0, 255); vertex(-1,  1, -1);
  	fill(255, 0, 255); vertex(-1, -1, -1);
  	fill(255, 0, 255); vertex( 1, -1, -1);

  	fill(255, 255, 0); vertex(-1,  1, -1);
  	fill(255, 255, 0); vertex(-1,  1,  1);
  	fill(255, 255, 0); vertex(-1, -1,  1);
  	fill(255, 255, 0); vertex(-1, -1, -1);


  	fill(255, 0, 0); vertex(-1,  1, -1);
  	fill(255, 0, 0); vertex( 1,  1, -1);
  	fill(255, 0, 0); vertex( 1,  1,  1);
  	fill(255, 0, 0); vertex(-1,  1,  1);


  	fill(0, 0, 255); vertex(-1, -1, -1);
  	fill(0, 0, 255); vertex( 1, -1, -1);
  	fill(0, 0, 255); vertex( 1, -1,  1);
  	fill(0, 0, 255); vertex(-1, -1,  1);
  	endShape();
    pop();

  }


}

function handleNotifications(event) {
  let value = event.target.value;
  for (let i = 0; i < value.byteLength; i++) {
      var s = '';
      s = String.fromCharCode(value.getUint8(i));

      notificationBuffer += s;
      if(s == '}'){
        currentReading = notificationBuffer.replace(/'/g,"\"");

        notificationBuffer = '';
        if(debug) print("New reading received " + currentReading);

        try {

          var reading = JSON.parse(currentReading);
          pitch= reading.Pitch;
          roll = reading.Roll;
          acceleration = new p5.Vector(reading.AccelX,reading.AccelY,reading.AccelZ);
        }
        catch (e) {
          print(e);
        }

      }
    }
  }

  // Tear down the bluetooth device
  function onDisconnected(event) {
    print('Bluetooth Device disconnected');

    if (txCharacteristic) {
      txCharacteristic.removeEventListener('characteristicvaluechanged', handleNotifications);
    }

    txCharacteristic = null;
    rxCharacteristic = null;
  }

  // Start pairing, connect using GATT, query charecteristics and setup a asynch event listenr for UART data
  function onStartButtonClick() {
    let serviceUuid = NUS_SERVICE_UUID;
    let tx_characteristicUuid = NUS_TX_CHARACTERISTIC_UUID;
    let rx_characteristicUuid = NUS_RX_CHARACTERISTIC_UUID;

    print('Requesting Bluetooth Device...');
    navigator.bluetooth.requestDevice({filters: [{services: [serviceUuid]}]})
    .then(device => {
      bluetoothDevice = device;
      bluetoothDevice.addEventListener('gattserverdisconnected', onDisconnected);

      print('Connecting to GATT Server...');
      return device.gatt.connect();
    })
    .then(server => {
      print('Getting Service...');
      return server.getPrimaryService(serviceUuid);
    })
    .then(service => {
      print('Getting Characteristics...');
      return Promise.all([
        service.getCharacteristic(tx_characteristicUuid)
          .then(characteristic => {
            txCharacteristic = characteristic;
            print('Tx charactersitic obtained');
            return txCharacteristic.startNotifications().then(_ => {
              print('Notifications started');
              txCharacteristic.addEventListener('characteristicvaluechanged', handleNotifications);
            });
          }),
        service.getCharacteristic(rx_characteristicUuid)
          .then(rx_characteristic => {
            rxCharacteristic = rx_characteristic;
            print('Rx charactersitic obtained');
          }),
      ]);
    })
    .catch(error => {
      print('Argh! ' + error);
    });
  }


  // Stop connection
  function onStopButtonClick() {
    if (txCharacteristic) {
      print('Stopping notifications...');
      try {
        txCharacteristic.removeEventListener('characteristicvaluechanged', handleNotifications);
      }
      catch(error) {
        print('Argh! ' + error);
      }
      txCharacteristic = null;
    }

    if (bluetoothDevice) {
      if (bluetoothDevice.gatt.connected) {
        print('Disconnecting from Bluetooth Device...');
        bluetoothDevice.gatt.disconnect();
      }
    }
  }


  // Test to see if experimental features are enabled
  function isWebBluetoothEnabled() {
    if (navigator.bluetooth) {
      return true;
    }
    else {
      print('Web Bluetooth API is not available.\n' +
        'Please make sure the Web Bluetooth flag is enabled.');
      return false;
    }
  }

  // Create a char array from a string
  function str2ab(str) {
    var buf = new ArrayBuffer(str.length);
    var bufView = new Uint8Array(buf);
    for (var i=0, strLen=str.length; i<strLen; i++) {
      bufView[i] = str.charCodeAt(i);
    }
    return buf;
  }
