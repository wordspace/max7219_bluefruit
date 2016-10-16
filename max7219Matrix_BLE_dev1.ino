/*********************************************************************
  This is an example for our nRF51822 based Bluefruit LE modules

  Pick one up today in the adafruit shop!

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  MIT license, check LICENSE for more information
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/

#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
#include <SoftwareSerial.h>
#endif

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

/*=========================================================================
    APPLICATION SETTINGS

      FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
     
                                Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                                running this at least once is a good idea.
     
                                When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                                Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
         
                                Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
#define FACTORYRESET_ENABLE         1
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

// Create the bluefruit object, either software serial...uncomment these lines
/*
  SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

  Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

int pinCS = 10; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI ) mosi=11, miso =12, sck=13 ss=10
int numberOfHorizontalDisplays = 6;
int numberOfVerticalDisplays = 1;

Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
int scrollspeed = 25; // 200 is  Default scrollspeed (milliseconds)

int spacer = 1;  // Space between two characters
int width = 5 + spacer; // The font width is 5 pixels
boolean inChar = false, NewData = false, pause = false;
boolean dataAvailable = false;
char inputString[512];

int count = 0, BTvalue = 5;

void display_data() {
  for ( int i = 0 ; i < width * count + matrix.width() - 1 - spacer; i++ ) {

    matrix.fillScreen(0);

    int letter = i / width;                       // "width = 6" letter only equals 1 on i = 6
    int x = (matrix.width() - 1) - i % width;     // goes from 0 to 6 incrementally
    int y = (matrix.height() - 8) / 2;            // center the text vertically  "this stays at zero"height =8, so 0/2 is 0

    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < count ) {
        matrix.drawChar(x, y, inputString[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }

    matrix.write(); // Send bitmap to display

    if (!dataAvailable) {
      i = width * count + matrix.width() - 1 - spacer;
      matrix.fillScreen(LOW);
      matrix.write();
    }
    if (NewData) {
      i = 0;
      matrix.fillScreen(LOW);
      matrix.write();
      NewData = false;
    }
    while (pause == true) {
      delay(0);
    } // Pause here if pause command is received

    delay(scrollspeed);
  }
}



void setup(void)
{
  matrix.setIntensity(10); // Use a value between 0 and 15 for brightness
  // Adjust to your own needs
  //matrix.setPosition(0, 7, 0); // The first display is at <0, 7>
  //matrix.setPosition(1, 6, 0); // The second display is at <1, 0>
  //matrix.setPosition(2, 5, 0); // The third display is at <2, 0>
  // matrix.setPosition(3, 4, 0); // And the last display is at <3, 0>
  // matrix.setPosition(4, 3, 0); // The first display is at <0, 0>
  // matrix.setPosition(5, 2, 0); // The second display is at <1, 0>
  // matrix.setPosition(6, 1, 0); // The third display is at <2, 0>
  //matrix.setPosition(7, 0, 0); // And the last display is at <3, 0>
  //  ...
  //  matrix.setRotation(0, 2);    // The first display is position upside down
  //  matrix.setRotation(3, 2);    // The same hold for the last display
  matrix.fillScreen(0);
  matrix.write();

  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit Command <-> Data Mode Example"));
  Serial.println(F("------------------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
    delay(500);
  }

  Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set module to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));
}

void loop(void)
{
  // Echo received data
  while (ble.available()) {
    char ch = ble.read();
    if (ch == '(') {
      count = 0;
      inChar = true;

      while (inChar) {
        if (ble.available()) {
          ch = ble.read();
          if (ch == ')') {
            inChar = false;
            dataAvailable = true;
          } else {
            inputString[count] = ch;
            count++;

          }
        }
      }
      display_data();
      display_data();
      display_data();
      display_data();
      display_data();
      display_data();
      dataAvailable = false;
      display_data();
    }
  }
}


