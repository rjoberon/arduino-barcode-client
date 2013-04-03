
#include <ctype.h>
#include <SPI.h>
#include <Ethernet.h>

/*
 * Reads barcodes from an attached barcode scanner and sends them to a server.
 * 
 * 
 *
 * TODO:
 * - remove polling code and use interrupts instead (see attachInterrupt)
 *
 * Changes:
 * 2011-12-13 (rja)
 * - initial version based on cuecatlog (http://www.instructables.com/id/Make-a-portable-barcode-scanner-with-an-arduino-an/)
 * 2012-01-02 (rja)
 * - cleaned up and commented code
 * 2012-01-03 (rja)
 * - implemented buffering of user name and simultaneous sending of name and code
 * 2012-01-04 (rja)
 * - migrated project to Eclipse
 * 2012-01-05 (rja)
 * - changed server IP to mercur
 */

/*
 * Configuration - Networking
 */
byte mac[]     = {0x90, 0xa2, 0xda, 0x00, 0x61, 0xf6}; // (arduino board)
byte ip[]      = {141, 51,167,149};
byte gateway[] = {141, 51,167,129};
byte subnet[]  = {255,255,255,192};
/*
 * Configuration - Networking (Server)
 */
IPAddress server(141,51,167,168);
int port = 4321;

/*
 * Configuration - Scanner
 */
int clockPin = 15; // Clock is only output. 
int dataPin  = 14; // The data pin is bi-directional  But at the moment we are only interested in receiving   
int ledPin   =  6; // When a SCAN_ENTER scancode is received the LED blink
int randPin  =  6; // this is the analog pin used for random numbers

/*
 * Constants - Scanner
 */
const int SCAN_ENTER = 0x5a; 
const int SCAN_BREAK = 0xf0;
const int QUANTITY_CODES = 50; // number of supported codes/characters
const byte SCAN_CODES[QUANTITY_CODES] = {0x70, 0x69, 0x72, 0x7a, 0x6b, 0x73, 0x74, 0x6c, 0x75, 0x7d, 0x1c, 0x32, 0x21, 0x23, 0x24, 0x2b, 0x34, 0x33, 0x43, 0x3b, 0x42, 0x4b, 0x3a, 0x31, 0x44, 0x4d, 0x15, 0x2d, 0x1b, 0x2c, 0x3c, 0x2a, 0x1d, 0x22, 0x35, 0x1a, 0x16, 0x1e, 0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46, 0x45, 0x29, 0x41, 0x49, 0x4a};
const char CHARACTERS[QUANTITY_CODES] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', ' ', ',', '.', '/'};

int breakActive = 0; // boolean toggled by SCAN_BREAK scan code
int clockValue = 0; 
byte dataValue;

const int BUFFER_LENGTH = 20;
char buffer[BUFFER_LENGTH];      // serial input buffer
int bufferPos = 0; 
uint8_t bufferidx = 0;
uint32_t tmp;
uint8_t i;

// buffer for scanned codes
char nameBuf[BUFFER_LENGTH];

// buffering scanned names
const unsigned long flushTime = 60000; // one minute
unsigned long lastNameScan;

// initialize ethernet client
EthernetClient client;

/*
 * function prototypes
 */
void clearBuffer(char *buf);
void sendData();
int readData();


/*
 * run once, when the sketch starts
 */
void setup() {
	Serial.begin(9600); // set up serial console for debug output
	Serial.println("barcode scanner log output");

	pinMode(dataPin, INPUT);  // data pin for scanner
	pinMode(clockPin, INPUT); // clock pin for scanner
	pinMode(ledPin, OUTPUT);
	randomSeed(analogRead(randPin));

	Ethernet.begin(mac, ip, gateway, subnet); // start ethernet connection

	Serial.println("ready!");
	Serial.println("waiting for incoming data");
}


/*
 * main program
 */
void loop() {

	// read data from scanner
	dataValue = readData();
	if (dataValue == SCAN_BREAK) {
		breakActive = 1;
	}

	// check for buffer overflow
	if (bufferPos < BUFFER_LENGTH) {
		// translate the scan codes to numbers
		// if there is a match, store it to the buffer
		for (int i = 0; i < QUANTITY_CODES; i++) {
			byte temp = SCAN_CODES[i];
			if (temp == dataValue) {
				if (!breakActive == 1) {
					buffer[bufferPos] = CHARACTERS[i];
					bufferPos++;
				} // end of breakActive
			} // end of "valid character"
		} // end of QUANTITY_CODES check
	}

	// code completely scanned
	if (dataValue == SCAN_ENTER) {
		Serial.print("scanned ");
		Serial.println(buffer);

		// flush name buffer, if necessary
		long diff = millis() - lastNameScan;
		if (diff < 0 || diff > flushTime) {
			Serial.print("flushing name buffer (age = ");
			Serial.print(diff / 1000);

			Serial.println(" seconds)");

			clearBuffer(nameBuf);
		}
		// successful scan of any code extends flush time
		lastNameScan = millis();


		// using the first character we decide, if a name or an EAN was scanned
		// TODO: improve decision process
		if (buffer[0] > 64) { // first character is a letter
			// name was scanned - always overwrite last scanned name

			Serial.println("name found, storing in name buffer");
			memcpy(nameBuf, buffer, bufferPos + 1);

		} else {
			// EAN was scanned - always send to server
			Serial.println("EAN found, sending to server");
			sendData();
		}

		// clear buffer
		clearBuffer(buffer);
		bufferPos = 0;

		// let LED blink
		digitalWrite(ledPin, HIGH);
		delay(300);
		digitalWrite(ledPin, LOW);
	} // end of "SCAN_ENTER" check

	// Reset the SCAN_BREAK state if the byte was a normal one
	if (dataValue != SCAN_BREAK) {
		breakActive = 0;
	}
	dataValue = 0;
} // end of loop()

/*
 * clears the given array with zeros
 */
void clearBuffer(char *buf) {
	Serial.println("clearing buffer");
	int i=0;
	while
		(buf[i] != 0) {
		buf[i++] = 0;
	} // end of clear buffer loop
}

/*
 * Sends an HTTP POST request containing the name and the code to the server.
 */
void sendData() {
	Serial.print("attempting to send data (name = ");
	Serial.print(nameBuf);
	Serial.print(", code = ");
	Serial.print(buffer);
	Serial.println(")");


	// if you get a connection, report back via serial:
	if (client.connect(server, port)) {
		Serial.println("connected");
		// Make a HTTP request:
		client.print("POST /barcode");
		client.print("?name=");
		client.print(nameBuf);
		client.print("&code=");
		client.print(buffer);
		client.print("&uptime=");
		client.print(millis());
		client.println(" HTTP/1.0");
		client.println();
		client.flush();
		client.stop();
		Serial.println("request sent");
	}
	else {
		// if you didn't get a connection to the server:
		Serial.println("connection failed");
	}
}

int readData() {
	byte val = 0;
	// Skip start state and start bit
	while (digitalRead(clockPin));   // Wait for LOW.
	// clock is high when idle
	while (!digitalRead(clockPin));  // Wait for HIGH.
	while (digitalRead(clockPin));  // Wait for LOW.
	for (int offset = 0; offset < 8; offset++) {
		while (digitalRead(clockPin));         // Wait for LOW
		val |= digitalRead(dataPin) << offset; // Add to byte
		while (!digitalRead(clockPin));        // Wait for HIGH
	}
	// Skipping parity and stop bits down here.
	while (digitalRead(clockPin));           // Wait for LOW.
	while (!digitalRead(clockPin));          // Wait for HIGH.
	while (digitalRead(clockPin));           // Wait for LOW.
	while (!digitalRead(clockPin));          // Wait for HIGH.
	return val;
}
