
#include "On.h"
#include "premiere.h"
#include "premieredeuxieme.h"
#include "premieretroisieme.h"
#include "premierequatrieme.h"
#include "deuxieme.h"
#include "deuxiemetroisieme.h"
#include "deuxiemequatrieme.h"
#include "troisieme.h"
#include "troisiemequatrieme.h"
#include "quatrieme.h"
#include "OFF.h"

// Comment out this line to remove all Debug information
#define debugFlag

// Comment out this line to remove the detailed Debug information
// #define deepDebugFlag

const char * userSSID = "Access Point";
const char * userPassword = "12345678";

enum : byte {temp_v, pression_v}; // in the same order as in the variables array
int variables[] = {22, 1013}; // default 22°C and 1013hPa for example
const byte maxVariables = sizeof(variables) / sizeof(variables[0]);

// The pin of your arduino connected to the reset of the ESP
const byte hardRestPIN = 7;

// The BAUD RATE OF YOUR ESP-01
// Don't go faster than 38400 with sowftare serial for reliability
#define ESPSERIALBAUD 115200 // Set to whatever is used by default by your ESP after a RESTORE

// uncomment this line if you want to use sowftare SERIAL and define the pins you use below
// #define USESOFTWARESERIAL

#ifdef USESOFTWARESERIAL
#include <SoftwareSerial.h>
// DEFINE THE PINS YOU USE FOR SOFTWARE SERIAL
#define ARDUINO_RX_PIN 15    // set here the pin number you use for software Serial Rx
#define ARDUINO_TX_PIN 14    // set here the pin number you use for software Serial Tx
SoftwareSerial esp01(ARDUINO_RX_PIN, ARDUINO_TX_PIN); // rxPin (the pin on which to receive serial data), txPin (the pin on which to transmit serial data)
#define ESPSEPRIAL esp01
#else
// DEFINE WHICH HARDWARE SERIAL YOU USE
#define ESPSEPRIAL Serial3  // could be Serial1, Serial2, Serial3 etc depending on your hardware.
#endif


// ******************************************************
// DON't CHANGE BELOW
// ******************************************************

byte myIPAddress[4];
uint16_t linkID;

const byte maxURLLength = 20;
char urlBuffer[maxURLLength];
int in2 = 7;

#ifdef debugFlag
#define debugMessage(...) Serial.print(__VA_ARGS__)
#else
#define debugMessage(...) {}
#endif


#ifdef deepDebugFlag
#define deepDebugMessage(...) Serial.print(__VA_ARGS__)
#else
#define deepDebugMessage(...) {}
#endif


#define SHORT_PAUSE (1000ul)
#define LONG_PAUSE  (10000ul)
const char * OK_STR = "OK\r\n";
const char * DOT_STR = ".";

const byte maxMessageSize = 100;
char ESP_MessageLine[maxMessageSize + 1]; // +1 as we want to be able to store the trailing '\0'

// =====================================================================================
// UTILITIES
// =====================================================================================

// --------------------------------------
// this will lock your arduino, watchdog might reset
// --------------------------------------
void dontGoFurther(const char * errorMessage)
{
  debugMessage(F("Can't continue: "));
  debugMessage(errorMessage);
  debugMessage(F("\n"));
  while (1); // die here
}


// --------------------------------------
// read a line from ESPSEPRIAL, ignore '\r' and
// returns true if '\n' is found
// --------------------------------------

boolean gotLine()
{
  static byte indexMessage = 0;
  boolean incomingMessage = true;

  while (ESPSEPRIAL.available() && incomingMessage) {
    int c = ESPSEPRIAL.read();
    if (c != -1) {
      switch (c) {
        case '\n':
          ESP_MessageLine[indexMessage] = '\0'; // trailing NULL char for a correct c-string
          indexMessage = 0; // get ready for next time
          incomingMessage = false;
          break;
        case '\r': // don't care about this one
          break;
        default:
          if (indexMessage <= maxMessageSize - 1) ESP_MessageLine[indexMessage++] = (char) c; // else ignore it..
          break;
      }
    }
  }
  return !incomingMessage;
}

// --------------------------------------                                               
// waitForString wait max for duration ms whilst checking if endMarker string is received
// on the ESP Serial port returns a boolean stating if the marker was found
// --------------------------------------

boolean waitForString(const char * endMarker, unsigned long duration)
{
  int localBufferSize = strlen(endMarker); // we won't need an \0 at the end
  char localBuffer[localBufferSize];
  int index = 0;
  boolean endMarkerFound = false;
  unsigned long currentTime;

  memset(localBuffer, '\0', localBufferSize); // clear buffer

  currentTime = millis();
  while (millis() - currentTime <= duration) {
    if (ESPSEPRIAL.available() > 0) {
      if (index == localBufferSize) index = 0;
      localBuffer[index] = (uint8_t) ESPSEPRIAL.read();
      deepDebugMessage(localBuffer[index]);
      endMarkerFound = true;
      for (int i = 0; i < localBufferSize; i++) {
        if (localBuffer[(index + 1 + i) % localBufferSize] != endMarker[i]) {
          endMarkerFound = false;
          break;
        }
      }
      index++;
    }
    if (endMarkerFound) break;
  }
  return endMarkerFound;
}

// --------------------------------------
// espPrintlnATCommand executes an AT commmand by adding at the end a CR LF
// then it checks if endMarker string is receivedon the ESP Serial port
// for max duration ms returns a boolean stating if the marker was found
// --------------------------------------

boolean espPrintlnATCommand(const char * command, const char * endMarker, unsigned long duration)
{
  debugMessage(DOT_STR);
  ESPSEPRIAL.println(command);
  return waitForString(endMarker, duration);
}

// --------------------------------------
// espPrintATCommand or espWriteATCommand is used when you don't want to send the CR LF
// at the end of the commmand line; use it to build up a multi part command
// same syntax as print as they are Variadic Macros using print or write
// --------------------------------------
#define espPrintATCommand(...) ESPSEPRIAL.print(__VA_ARGS__)
#define espWriteATCommand(...) ESPSEPRIAL.write(__VA_ARGS__)


// --------------------------------------
// extract from the flow of data our IP address
// --------------------------------------
boolean getMyIPAddress()
{
  unsigned long currentTime;

  const char * searchForString = "+CIFSR:STAIP,";
  const byte searchForStringLength = strlen(searchForString);

  char * ptr;
  boolean foundMyIPAddress = false;

  espPrintATCommand(F("AT+CIFSR\r\n")); // ask for our IP address

  // this returns something like
  //  AT+CIFSR
  //
  //  +CIFSR:STAIP,"192.168.1.28"
  //  +CIFSR:STAMAC,"18:fe:34:e6:27:8f"
  //
  //  OK

  currentTime = millis();
  while (millis() - currentTime <= LONG_PAUSE) {
    if (gotLine()) {
      if (ptr = strstr(ESP_MessageLine, searchForString)) {
        ptr += searchForStringLength + 1; // +1 to skip the "
        if (!(ptr = strtok (ptr, DOT_STR))) break;
        myIPAddress[0] = atoi(ptr);
        if (!(ptr = strtok (NULL, DOT_STR))) break;
        myIPAddress[1] = atoi(ptr);
        if (!(ptr = strtok (NULL, DOT_STR))) break;
        myIPAddress[2] = atoi(ptr);
        if (!(ptr = strtok (NULL, DOT_STR))) break;
        myIPAddress[3] = atoi(ptr);
        if (foundMyIPAddress = waitForString(OK_STR, SHORT_PAUSE)) { // wait for the final OK
          debugMessage(F("\nmy IP address is: "));
          debugMessage(myIPAddress[0]); debugMessage(DOT_STR);
          debugMessage(myIPAddress[1]); debugMessage(DOT_STR);
          debugMessage(myIPAddress[2]); debugMessage(DOT_STR);
          debugMessage(myIPAddress[3]);
          debugMessage(F("\n"));
          break;
        }
      }
    }
  }
  return foundMyIPAddress ;
}

// wait for something like  "+IPD,<linkID>,<someSize>:GET <URL> HTTP/1.1"
// extract the linkID (integer) and URL (string)
boolean isHTTPRequest(char * s)
{
  boolean correct = false;
  char * ptr1, *ptr2, *ptr3;

  deepDebugMessage(F("Parsing ["));
  deepDebugMessage(s);
  deepDebugMessage(F("]\n"));

  if (ptr1 = strstr(s, "+IPD,")) {
    if (ptr2 = strstr(s, ":GET /")) {
      if (ptr3 = strstr(s, " HTTP")) {
        linkID = atoi(ptr1 + 5);
        byte b = *ptr3;
        *ptr3 = '\0';
        strncpy(urlBuffer, ptr2 + 5, maxURLLength);
        urlBuffer[maxURLLength - 1] = '\0'; // strncpy might not put the trailing null
        *ptr3 = b;   // restore string as it was

        if (correct = waitForString("\r\n\r\n", SHORT_PAUSE)) { // wait for the empty line
          debugMessage(F("\nFound HTTP GET Request. my Link ID is "));
          debugMessage(linkID);
          debugMessage(F(" for URL ["));
          debugMessage(urlBuffer);
          debugMessage(F("]\n"));
        }
      }
    }
  }
  return correct;
}


void Allumer()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(On);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(On + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}
// ------------------------------------------------------------------


void Eteindre()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(OFF);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(OFF + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------

void grillee1()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(premiere);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(premiere + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------


void grillee12()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(premieredeuxieme);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(premieredeuxieme + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------

void grillee13()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(premieretroisieme);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(premieretroisieme + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------

void grillee14()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(premierequatrieme);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(premierequatrieme + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------

void grillee2()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(deuxieme);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(deuxieme + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------

void grillee23()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(deuxiemetroisieme);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(deuxiemetroisieme + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------


void grillee24()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(deuxiemequatrieme);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(deuxiemequatrieme + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------



void grillee3()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(troisieme);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(troisieme + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------

void grillee34()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(troisiemequatrieme);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(troisiemequatrieme + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------



void grillee4()
{
  // our HTML is in the webRoot[] array
  const byte maxLine = 100;
  char lineToScan[maxLine + 1];
  uint8_t c, index;

  char lineToSend[maxLine + 20]; // a bit bigger to fit our variable just in case (hack)
  const char * key1 = "$$$";

  uint16_t nbBytes = strlen_P(quatrieme);
  index = 0;

  for (uint16_t pos = 0; pos < nbBytes; pos++) {
    c =  pgm_read_byte(quatrieme + pos);
    lineToScan[index++] = (char) c;

    if ((c == '\n') || (index >= maxLine - 1)) {
      lineToScan[index] = '\0';
      index = 0;

      // replace all occurences of $$$n$$$ by it's value. Handle only one per line
      // this is a quick hack would need to be optimized
      // for example sending in 3 pieces would save the second buffer
      char *ptr1, *ptr2;
      boolean replaced = false;

      if (ptr1 = strstr(lineToScan, key1)) {
        if (ptr2 = strstr(ptr1 + 3, key1)) {
          int v = atoi(ptr1 + 3);
          if (v >= 0 && v <= maxVariables) {
            *ptr1 = '\0';
            strcpy(lineToSend, lineToScan);
            itoa(variables[v], &lineToSend[strlen(lineToScan)], 10); // risk of overflow
            strcpy(lineToSend + strlen(lineToSend), ptr2 + 3);  // risk of overflow
            replaced = true;
          }
        }
      }
      if (! replaced) strcpy(lineToSend, lineToScan);

      if (!strcmp(lineToSend, "\n"))
        strcpy(lineToSend, "\r\n");        // respect the HTTP spec for empty line and send \r\n instead

      espPrintATCommand(F("AT+CIPSEND="));
      espPrintATCommand(linkID);
      espPrintATCommand(F(","));
      espPrintATCommand(strlen(lineToSend));
      if (!espPrintlnATCommand("", ">", LONG_PAUSE)) {
        debugMessage(F("failed to get prmpot for CIPSEND!\n"));
      } else {
        espPrintATCommand(lineToSend);
        if (!waitForString(OK_STR, SHORT_PAUSE)) {
          debugMessage(F("failed to CIPSEND: "));
          debugMessage(lineToScan);
          break;
        }
        delay(22); // CIPSEND spec states 20ms between two packets
      }
    }
  }
}

// ------------------------------------------------------------------



void setup() {
  Serial.begin(115200); while (!Serial);
  ESPSEPRIAL.begin(ESPSERIALBAUD); while (!ESPSEPRIAL);
  pinMode (in2, OUTPUT);
  digitalWrite (in2, HIGH);


  pinMode(LED_BUILTIN, OUTPUT);

  //reset the esp01 (pull the RST pin to ground for 100ms)
  digitalWrite(hardRestPIN, LOW);
  delay(100);
  digitalWrite(hardRestPIN, HIGH);

  // connect to my WiFi network and create the TCP server
  if (!espPrintlnATCommand("AT+RST", "ready", LONG_PAUSE)) dontGoFurther("RESTORE failed"); // reset

  if (!espPrintlnATCommand("AT+CWMODE=1", OK_STR, SHORT_PAUSE)) dontGoFurther("CWMODE failed"); //Set the wireless mode to station

  if (!espPrintlnATCommand("AT+CWQAP", OK_STR, SHORT_PAUSE)) dontGoFurther("CWQAP failed");   //disconnect  - it shouldn't be but just to make sure

  espPrintATCommand(F("AT+CWJAP=\""));
  espPrintATCommand(userSSID);
  espPrintATCommand(F("\",\""));
  espPrintATCommand(userPassword);
  if (!espPrintlnATCommand("\"", OK_STR, LONG_PAUSE)) dontGoFurther("CWJAP failed");   // connect to wifi

  if (!getMyIPAddress()) dontGoFurther("CIFSR failed"); //get our IP address

  if (!espPrintlnATCommand("AT+CIPMUX=1", OK_STR, SHORT_PAUSE)) dontGoFurther("CIPMUX failed"); //set the multi connection mode

  if (!espPrintlnATCommand("AT+CIPSERVER=1,80", OK_STR, SHORT_PAUSE)) dontGoFurther("CIPSERVER failed"); //create the server

  debugMessage(F("\n\nReady!\n"));
}




void loop() {
  int valeur1 = analogRead(A1);
  int valeur2 = analogRead(A2);
  int valeur3 = analogRead(A3);
  int valeur4 = analogRead(A4);
  
  if (gotLine()) {
    if (isHTTPRequest(ESP_MessageLine)) {
      if (!strcmp(urlBuffer, "/")) {
       Eteindre();
      } else if (!strcmp(urlBuffer, "/Allumer")) {
        digitalWrite (in2, LOW);
        if (valeur1<900 && valeur2>900  && valeur3>900  && valeur4>900 ){
          grillee1();
        } else if (valeur1>900 && valeur2<900  && valeur3>900  && valeur4>900 ){
          grillee2();
        } else if (valeur1>900 && valeur2>900  && valeur3<900  && valeur4>900 ){
          grillee3();
        } else if (valeur1>900 && valeur2>900  && valeur3>900  && valeur4<900 ){
          grillee4();
        } else if (valeur1<900 && valeur2>900  && valeur3>900  && valeur4<900 ){
          grillee14();
        } else if (valeur1<900 && valeur2>900  && valeur3<900  && valeur4>900 ){
          grillee13();
        } else if (valeur1<900 && valeur2<900  && valeur3>900  && valeur4>900 ){
          grillee12();
        }  else if (valeur1>900 && valeur2<900  && valeur3<900  && valeur4>900 ){
          grillee23();
        }  else if (valeur1>900 && valeur2<900  && valeur3>900  && valeur4<900 ){
          grillee24();
        } else if (valeur1>900 && valeur2>900  && valeur3<900  && valeur4<900 ){
          grillee34();
        }else {
          Allumer();
        }
        
      } else if (!strcmp(urlBuffer, "/Eteindre")) {
        digitalWrite (in2, HIGH);
        Eteindre();
      } 

      espPrintATCommand(F("AT+CIPCLOSE="));
      espPrintATCommand(linkID);
      if (!espPrintlnATCommand("", OK_STR, LONG_PAUSE)) debugMessage(F("failed to CIPCLOSE!\n"));
    }
  }

  // can do other stuff here as long as it's not too long

  // for example we blink the built in LED at the rate of the temperature variable x 10
  static unsigned long chrono = 0;
  if (millis() - chrono >= variables[temp_v] * 10ul) {
    if (digitalRead(LED_BUILTIN) == LOW)
      digitalWrite(LED_BUILTIN, HIGH);
    else
      digitalWrite(LED_BUILTIN, LOW);

    chrono = millis();
  }
}
