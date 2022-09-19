#include <MKRWAN.h>

#include <Wire.h>

LoRaModem modem;

float previous_voltage;
float current_voltage = -1.0;
bool hi_lo = false;
bool lo_hi = false;
bool first = true;

#include "arduino_secrets.h"
// Please enter your sensitive data in the Secret tab or arduino_secrets.h
String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  // change this to your regional band (eg. US915, AS923, ...)
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
    while (1) {}
  };

  int connected = modem.joinOTAA(appEui, appKey);
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    while (1) {}
  }



  // Set poll interval to 60 secs.
  modem.minPollInterval(60);

  Wire.begin(8);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // function that executes whenever data is received from writer
  Serial.println("All set up, ready to go.");
}

void loop() {
  if (hi_lo || lo_hi) {
    if (hi_lo) {
      Serial.println("Power failure!");
    } else {
      Serial.println("Power returned!");
    }
    Serial.print ("Voltage: "); Serial.println (current_voltage);
    String msg = String(current_voltage);
    Serial.println();
    Serial.println("Sending now: " + msg + " - ");
    for (unsigned int i = 0; i < msg.length(); i++) {
      Serial.print(msg[i] >> 4, HEX);
      Serial.print(msg[i] & 0xF, HEX);
      Serial.print(" ");
    }
    Serial.println();

    int err;
    modem.beginPacket();
    modem.print(msg);
    err = modem.endPacket(true);

    Serial.println("Status: " + err);
    if (err > 0) {
      Serial.println("Message sent correctly!");
      if (hi_lo) {
        hi_lo = false;
      } else {
        lo_hi = false;
      }
    } else {
      Serial.println("Error sending message :(");
      Serial.println("(you may send a limited amount of messages per minute, depending on the signal strength");
      Serial.println("it may vary from 1 message every couple of seconds to 1 message every minute)");
    }
  }
  //Serial.println("Taking a nap.");
  //delay(60500);
} 

void receiveEvent(int howMany) {
  if (howMany != 4) {
    Serial.println("Received invalid message (too short or too long)");
    String msg = "No message trying to be sent";
    Serial.println("Sending: " + msg + " - ");
  } else {
    union {
      float voltage;
      byte v_arr[4];
    } myData;
    for (int i = 0; i < 4; i++) {
      myData.v_arr[i] = Wire.read();
    }
    previous_voltage = current_voltage;
    current_voltage = myData.voltage;
    if (first) {
      first = false;
    } else {
      if (previous_voltage < 100.0 && current_voltage > 100.0) {
        lo_hi = true;
      } else if (previous_voltage > 100.0 && current_voltage < 100.0) {
        hi_lo = true;
      }
    }
  }
}
