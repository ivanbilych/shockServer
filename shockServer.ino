#include <SPI.h>
#include <Ethernet.h>

#define SERIAL_SPEED   115200
#define KEY_MODE_CH    'm'
#define KEY_OK_CH      'o'
#define STOP_SERVER_CH 's'

enum {
  KEY_MODE = 0,
  KEY_OK
};

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte keyGPIO[] = { 2, 3 };
byte stopServer = 3;

IPAddress ip(192, 168, 0, 80);
IPAddress myDns(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(23);

boolean alreadyConnected = false;

void setupKeys() {
  int size = sizeof(keyGPIO) / sizeof(keyGPIO[0]);

  for ( int i = 0; i < size; i++ ) {
    pinMode(keyGPIO[i], OUTPUT);
    digitalWrite(keyGPIO[i], HIGH);
  }
}

void setupConnection() {
  Ethernet.begin(mac, ip, myDns, gateway, subnet);
  server.begin();
}

void setupSerial() {
  Serial.begin(SERIAL_SPEED);
  while ( !Serial ) { ; }

  Serial.print("Starting server on: ");
  Serial.println(Ethernet.localIP());
}

void emulateKeypress(byte key) {
  Serial.print("Pressing key #");
  Serial.println(key);

  digitalWrite(keyGPIO[key], LOW);
  delay(1500);
  digitalWrite(keyGPIO[key], HIGH);
}

void proceedKeypress(char command, EthernetClient& client) {
  switch ( command ) {
    case KEY_MODE_CH :
      emulateKeypress(KEY_MODE);
      break;
    case KEY_OK_CH :
      emulateKeypress(KEY_OK);
      break;
    case STOP_SERVER_CH :
      stopServer -= 1;
      if ( stopServer == 0 ) {
        Serial.print("Stopping Server!");
        client.println("Stopping Server!");
        for (;;) {}
      }
      break;
  }
}

void setup() {
  setupKeys();
  setupConnection();
  setupSerial();
}

void loop() {
  EthernetClient client = server.available();

  if (client) {
    if ( !alreadyConnected ) {
      client.flush();
      Serial.println("We have a new client!");
      client.println("You are connected to shock server");
      alreadyConnected = true;
    }

    if ( client.available() > 0 ) {
      proceedKeypress(client.read(), client);
    }
  }
}

