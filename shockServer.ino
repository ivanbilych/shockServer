#include <SPI.h>
#include <Ethernet.h>

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])

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
#define DISCONNECT_CH   'q'
#define HELP_CH         'h'

enum {
    KEY_MODE = 0,
    KEY_OK
};

int shockAttempts = 0;

char shockPhrase[] = { 's', 'h', 'o', 'c', 'k' };
char restartPhrase[] = { 'r', 'e', 's', 't', 'a', 'r', 't' };

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte keyGPIO[] = { 2, 3 };

IPAddress ip(192, 168, 0, 80);
IPAddress mask(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 1);
IPAddress serverDns(192, 168, 0, 1);
EthernetServer server(TELNET_PORT);
EthernetClient client;

boolean alreadyConnected = false;

void (* resetFunc) (void) = 0;

void setupSerial() {
    Serial.begin(SERIAL_SPEED);
    while ( !Serial ) { }

    SERIAL_PRINT("Serial setup is complete\n");
}

void setupGPIO() {
    int size = sizeof(keyGPIO) / sizeof(keyGPIO[0]);

    for ( int i = 0; i < size; i++ ) {
        pinMode(keyGPIO[i], OUTPUT);
        digitalWrite(keyGPIO[i], HIGH);
    }

    SERIAL_PRINT("GPIO setup is complete\n");
}

void setupConnection() {
    Ethernet.begin(mac, ip, serverDns, gateway, mask);
    server.begin();

    SERIAL_PRINT("Starting server on: ");
    SERIAL_PRINT(Ethernet.localIP());
    SERIAL_PRINT("\n");

    SERIAL_PRINT("Connection setup is complete\n");
}

void setup() {
#ifdef DEBUG
    setupSerial();
#endif
    setupGPIO();
    setupConnection();
}

int confirmation(char* phrase, int size) {
    char userPhrase[size];

    for ( ; !client.available(); ) {}

    for ( int i = 0; i < size; i++ ) {
        userPhrase[i] = client.read();
    }

    for ( int i = 0; i < size; i++ ) {
        if ( phrase[i] != userPhrase[i] ) {
            PRINT("Wrong command\n");

            return 0;
        }
    }

    return 1;
}

void emulateKeypress(byte key) {
    digitalWrite(keyGPIO[key], LOW);
    delay(KEYPRESS_DELAY_MS);
    digitalWrite(keyGPIO[key], HIGH);
    PRINT("Key ");
    PRINT(key);
    PRINT(" pressed\n");
}

void getStats() {;
    PRINT("Number of shock attempts: ");
    PRINT(shockAttempts);
    PRINT("\n");
}

void printHelp() {
    PRINT("Server commands:\n");
    PRINT("m - MODE button press\n");
    PRINT("o - OK button press [confirmation required]\n");
    PRINT("r - Restart server [confirmation required]\n");
    PRINT("s - Show stats\n");
    PRINT("q - Disconnect client\n");
    PRINT("h - Print this help\n");
}

void proceedKeypress() {
    char command = client.read();
    client.read();
    client.read();

    switch ( command ) {
    case KEY_MODE_CH:
        emulateKeypress(KEY_MODE);
        break;

    case KEY_OK_CH:
        PRINT("Enter 'shock' if you really want it\n");

        if ( confirmation(shockPhrase, ARRAY_SIZE(shockPhrase)) ) {
            emulateKeypress(KEY_OK);
            shockAttempts += 1;
        }
        break;

    case RESET_SERVER_CH:
        PRINT("Enter 'restart' if you really want it\n");

        if ( confirmation(restartPhrase, ARRAY_SIZE(restartPhrase)) ) {
            PRINT("RESTARTING SERVER IN 5 SEC\n");

            delay(SERVER_RESTART_DELAY_MS);

            resetFunc();
        }
        break;

    case GET_STATS_CH:
        getStats();
        break;

    case DISCONNECT_CH:
        PRINT("Disconnecting client\n");

        client.stop();
        alreadyConnected = false;
        break;

    case HELP_CH:
        printHelp();
        break;
    }
}

void loop() {
    client = server.available();

    if ( client ) {
        if ( !alreadyConnected ) {
            client.flush();

            printHelp();

            alreadyConnected = true;
        }

        if ( client.available() ) {
            proceedKeypress();
        }
    }
}
