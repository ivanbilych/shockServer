#include <SPI.h>
#include <Ethernet.h>

#define DEBUG

#ifdef DEBUG
#define SERIAL_PRINT(x) Serial.print(x);
#else
#define SERIAL_PRINT(x)
#endif
#define TELNET_PRINT(x) client.print(x);
#define PRINT(x) TELNET_PRINT(x); SERIAL_PRINT(x);

#define TELNET_PORT             23
#define SERIAL_SPEED            115200
#define KEYPRESS_DELAY_MS       1500
#define SERVER_RESTART_DELAY_MS 5000

#define KEY_MODE_CH     'm'
#define KEY_OK_CH       'o'
#define RESET_SERVER_CH 'r'
#define GET_STATS_CH    's'

enum {
    KEY_MODE = 0,
    KEY_OK
};

int shockAttempts = 0;
char passwordServer[] = { 'd', 'o', 'g' };
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte keyGPIO[] = { 2, 3 };
byte serverResetCount = 0;

IPAddress ip(192, 168, 0, 80);
IPAddress mask(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 1);
IPAddress serverDns(192, 168, 0, 1);
EthernetServer server(TELNET_PORT);
EthernetClient client;

boolean alreadyConnected = false;
boolean handshakeDone = false;

void(* resetFunc) (void) = 0;

void setupSerial() {
    Serial.begin(SERIAL_SPEED);
    while ( !Serial ) { }

    SERIAL_PRINT("Starting server on: ");
    SERIAL_PRINT(Ethernet.localIP());
    SERIAL_PRINT("\n");
}

void setupConnection() {
    Ethernet.begin(mac, ip, serverDns, gateway, mask);
    server.begin();

    SERIAL_PRINT("Connection setup is complete\n");
}

void setupGPIO() {
    int size = sizeof(keyGPIO) / sizeof(keyGPIO[0]);

    for ( int i = 0; i < size; i++ ) {
        pinMode(keyGPIO[i], OUTPUT);
        digitalWrite(keyGPIO[i], HIGH);
    }

    SERIAL_PRINT("GPIO setup is complete\n");
}

void setup() {
#ifdef DEBUG
    setupSerial();
#endif
    setupConnection();
    setupGPIO();
}

void handshake() {
    int size = sizeof(passwordServer) / sizeof(passwordServer[0]);
    char passwordUser[size];

    PRINT("Password required\n");
    PRINT(size);
    PRINT(" symbols\n");

    for ( int i = 0; i < size; i++ ) {
        passwordUser[i] = client.read();
    }

    for ( int i = 0; i < size && passwordServer[i] == passwordUser[i]; i++ ) {
        if ( passwordServer[i] != passwordUser[i] ) {
            PRINT("Access denied\n");

            return;
        }
    }

    PRINT("Access granted\n");

    handshakeDone = true;
}

void emulateKeypress(byte key) {
    SERIAL_PRINT("Pressing key #");
    SERIAL_PRINT(key);
    SERIAL_PRINT("\n");

    digitalWrite(keyGPIO[key], LOW);
    delay(KEYPRESS_DELAY_MS);
    digitalWrite(keyGPIO[key], HIGH);
}

void restartServer() {
    if ( serverResetCount == 0 ) {
        PRINT("WARNING! You are going to restart server\n");
    } else if ( serverResetCount == 1 ) {
        PRINT("WARNING! Server will be restarted on next attempt\n");
    } else {
        PRINT("RESTARTING SERVER IN 5 SEC\n");

        delay(SERVER_RESTART_DELAY_MS);
        resetFunc();
    }

    serverResetCount += 1;
}

void getStats() {
    PRINT("Server statistics:\n");
    PRINT("Number of shock attempts:");
    PRINT(shockAttempts);
    PRINT("\n");
}

void proceedKeypress() {
    char command = client.read();

    switch ( command ) {
    case KEY_MODE_CH:
        emulateKeypress(KEY_MODE);
        break;

    case KEY_OK_CH:
        emulateKeypress(KEY_OK);
        shockAttempts += 1;
        break;

    case RESET_SERVER_CH:
        restartServer();
        break;

    case GET_STATS_CH:
        getStats();
        break;

    default:
        PRINT("Unknown command\n");
    }
}

void loop() {
    client = server.available();

    if (client) {
        if ( !alreadyConnected ) {
            client.flush();

            PRINT("Client connected to server\n");

            alreadyConnected = true;
        }

        if ( client.available() && !handshakeDone ) {
            handshake();
        } else if ( client.available() && handshakeDone ) {
            proceedKeypress();
        }
    } else {
        handshakeDone = false;
    }
}
