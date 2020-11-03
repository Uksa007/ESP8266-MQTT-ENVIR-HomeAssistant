// ------------------------------
// ---- all config in auth.h ----
// ------------------------------
#define VERSION F("v6.9 - EnergyProxy - https://github.com/DotNetDann - http://dotnetdan.info")

/* Initialise serial appropriately */
#define CC_BAUD 57600

#include <PubSubClient.h> //https://pubsubclient.knolleary.net/
#include <SPI.h> //http://playground.arduino.cc/Code/Spi
#include <TimeLib.h> //http://playground.arduino.cc/Code/time - https://github.com/PaulStoffregen/Time
#include "auth.h"

#ifdef _ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiServer.h>
  #include <WiFiClient.h>
  #include <WiFiClientSecure.h>
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
  #include <SoftwareSerial.h> //EspSoftwareSerial
  SoftwareSerial ccSerial(13, 15, false); // D7 and D8 RX,TX
  #define CC_SERIAL ccSerial
  #define NETWORK WiFi
#endif
#ifdef _CCBRIDGE
  #include <Ethernet.h>
  #include <EthernetUdp.h>
  #define CC_SERIAL Serial
  #define ETH_PIN 7
  #define NETWORK Ethernet
#endif
#ifdef _ETHERTEN
  #include <Ethernet.h>
  #include <EthernetUdp.h>
  #include <SoftwareSerial.h>
  #define SERIAL_RX 2
  #define SERIAL_TX 3
  SoftwareSerial ccSerial(SERIAL_RX, SERIAL_TX);
  #define CC_SERIAL ccSerial
  #define NETWORK Ethernet
#endif

/* The size of the buffer that messages are read onto */
/* (should fit a whole message) */
/* 166-288 (One is 332 and history ones are larger) */
#define BUFFER_SIZE 350

/* Max time to wait between bits of a message (milliseconds) */
#define MSG_DELAY 400

/* ~~~~~~~~~~~~~~~~ */
/* Global Variables */
/* ~~~~~~~~~~~~~~~~ */
int    lastHour;
time_t dateStarted;
time_t dateFailed;

/* Networking details (add your ethernet shield's MAC address) */

#ifdef _ESP8266
  WiFiClient client[3];
  WiFiClient webClient;
  WiFiClientSecure secureclient;
  //WiFiServer server(80);
  ESP8266WebServer server(80);
  WiFiUDP Udp;
  HTTPClient http;
#else
  byte mac[] = {0x90, 0xA2, 0xDA, 0x02, 0x03, 0xC5};
  byte ip[] = {192, 168, 30, 200};
  byte gateway[] = {192, 168, 30, 1};
  EthernetClient client[3];
  EthernetServer server(80);              // the web server is used to serve status calls
  EthernetUDP Udp;
  HttpClient http;
#endif

/* Message buffer & its counter */
char buffer[BUFFER_SIZE];
int i = 0;

/* The time of the last read from the serial */
unsigned long t_lastread = 0;

boolean overflowed = false;

/* Watch the connection quality */
int failed_connections = 0;

// DigiCert High Assurance EV Root CA
const char trustRoot[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIGCDCCA/CgAwIBAgIQKy5u6tl1NmwUim7bo3yMBzANBgkqhkiG9w0BAQwFADCB
hTELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G
A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxKzApBgNV
BAMTIkNPTU9ETyBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTQwMjEy
MDAwMDAwWhcNMjkwMjExMjM1OTU5WjCBkDELMAkGA1UEBhMCR0IxGzAZBgNVBAgT
EkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEaMBgGA1UEChMR
Q09NT0RPIENBIExpbWl0ZWQxNjA0BgNVBAMTLUNPTU9ETyBSU0EgRG9tYWluIFZh
bGlkYXRpb24gU2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEP
ADCCAQoCggEBAI7CAhnhoFmk6zg1jSz9AdDTScBkxwtiBUUWOqigwAwCfx3M28Sh
bXcDow+G+eMGnD4LgYqbSRutA776S9uMIO3Vzl5ljj4Nr0zCsLdFXlIvNN5IJGS0
Qa4Al/e+Z96e0HqnU4A7fK31llVvl0cKfIWLIpeNs4TgllfQcBhglo/uLQeTnaG6
ytHNe+nEKpooIZFNb5JPJaXyejXdJtxGpdCsWTWM/06RQ1A/WZMebFEh7lgUq/51
UHg+TLAchhP6a5i84DuUHoVS3AOTJBhuyydRReZw3iVDpA3hSqXttn7IzW3uLh0n
c13cRTCAquOyQQuvvUSH2rnlG51/ruWFgqUCAwEAAaOCAWUwggFhMB8GA1UdIwQY
MBaAFLuvfgI9+qbxPISOre44mOzZMjLUMB0GA1UdDgQWBBSQr2o6lFoL2JDqElZz
30O0Oija5zAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADAdBgNV
HSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwGwYDVR0gBBQwEjAGBgRVHSAAMAgG
BmeBDAECATBMBgNVHR8ERTBDMEGgP6A9hjtodHRwOi8vY3JsLmNvbW9kb2NhLmNv
bS9DT01PRE9SU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDBxBggrBgEFBQcB
AQRlMGMwOwYIKwYBBQUHMAKGL2h0dHA6Ly9jcnQuY29tb2RvY2EuY29tL0NPTU9E
T1JTQUFkZFRydXN0Q0EuY3J0MCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21v
ZG9jYS5jb20wDQYJKoZIhvcNAQEMBQADggIBAE4rdk+SHGI2ibp3wScF9BzWRJ2p
mj6q1WZmAT7qSeaiNbz69t2Vjpk1mA42GHWx3d1Qcnyu3HeIzg/3kCDKo2cuH1Z/
e+FE6kKVxF0NAVBGFfKBiVlsit2M8RKhjTpCipj4SzR7JzsItG8kO3KdY3RYPBps
P0/HEZrIqPW1N+8QRcZs2eBelSaz662jue5/DJpmNXMyYE7l3YphLG5SEXdoltMY
dVEVABt0iN3hxzgEQyjpFv3ZBdRdRydg1vs4O2xyopT4Qhrf7W8GjEXCBgCq5Ojc
2bXhc3js9iPc0d1sjhqPpepUfJa3w/5Vjo1JXvxku88+vZbrac2/4EjxYoIQ5QxG
V/Iz2tDIY+3GH5QFlkoakdH368+PUq4NCNk+qKBR6cGHdNXJ93SrLlP7u3r7l+L4
HyaPs9Kg4DdbKDsx5Q5XLVq4rXmsXiBmGqW5prU5wfWYQ//u+aen/e7KJD2AFsQX
j4rBYKEMrltDR5FL1ZoXX/nUh8HCjLfn4g8wGTeGrODcQgPmlKidrv0PJFGUzpII
0fxQ8ANAe4hZ7Q7drNJ3gjTcBpUC2JD5Leo31Rpg0Gcg19hCC0Wvgmje3WYkN5Ap
lBlGGSW4gNfL1IYoakRwJiNiqZ+Gb7+6kHDSVneFeO/qJakXzlByjAA6quPbYzSf
+AZxAeKCINT+b72x
-----END CERTIFICATE-----
)EOF";
X509List cert(trustRoot);

PubSubClient mqtt(client[2]);

/* ~~~~~~~~~~~~ */
/* Program Body */
/* ~~~~~~~~~~~~ */

#ifdef _DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

/* All the real work is in the xml processor */
#include "resultproc.h"
#include "xmlproc.h"
#include "ntp-time.h"
#include "web-client.h"

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);       // Initialize the LED_BUILTIN pin as an output (So it doesnt float as a LED is on this pin)
  digitalWrite(LED_BUILTIN, LOW);     // Turn the status LED on

  /* Initialise Arduino to CurrentCost meter serial */
  CC_SERIAL.begin(CC_BAUD);

#ifdef _DEBUG
  /* Opens debug serial port */
  Serial.begin(115200);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN(F("Starting.."));
#endif

#ifdef _BRIDGE
  pinMode(ETH_PIN, OUTPUT);
  digitalWrite(ETH_PIN, HIGH);
#endif

  /* Connect to the network */
#ifdef _ESP8266
  NETWORK.begin(WIFI_SSID, WIFI_PASSWORD);
  DEBUG_PRINT(F("Connecting to "));
  DEBUG_PRINTLN(WIFI_SSID);
  
  while (NETWORK.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname("EnviR"); // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setPassword((const char *)OTApassword); // No authentication by default

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
 
#else
  if (NETWORK.begin(mac) == 0)
  {
    DEBUG_PRINTLN(F("DHCP failed!"));
    NETWORK.begin(mac, ip, gateway, gateway);
  }

  NETWORK.maintain();
#endif

  DEBUG_PRINT(F("Local IP: "));
  DEBUG_PRINTLN(NETWORK.localIP());
    
  // initialize time server
  Udp.begin(8888);
  DEBUG_PRINTLN(F("Setting time using NTP"));
  while(!UpdateTime()); // wait until time is set
  DEBUG_PRINT(F("Time is "));
  SetDateTime(now());
  DEBUG_PRINTLN(fdata);
  
  #if MQTT_ENABLE == 1
  mqtt.setServer(MQTT_SERVER, 1883);
  #endif

  server.on("/", ServeWebClients);
  server.begin();
  
  lastHour = hour();
  dateStarted = now();
  dateFailed = now();

  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off
}


void loop()
{
  /* Read and send data */
  ReadMeter();
  
  /* Reconnect to the network if neccessary */
  if (failed_connections > 3) {
    DEBUG_PRINT(F("Failed Connections - Reset"));
    failed_connections = 0;
    client[0].flush();
    client[0].stop();

    //if (network.begin(mac) == 0) {
    //  DEBUG_PRINTLN(F("DHCP failed!"));
    //  network.begin(mac, ip, gateway, gateway);
    //}
  }

  /* Update local time/date */
  if(hour()!=lastHour) {
    lastHour=hour();
    if(lastHour==10 || lastHour==22) {
      UpdateTime();
    }
  }

  ArduinoOTA.handle(); // Check OTA Firmware Updates
  
  server.handleClient(); // Check Web page requests

  #ifdef _STATUSLED
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off
  #endif
  
  delay(50);
}

void ReadMeter()
{
  /*
     The incoming message appears on the SoftwareSerial buffer
     in several parts separated by small time intervals.
     Wait for the whole message to arrive before processing
  */
  t_lastread = millis();
  while ((millis() - t_lastread < MSG_DELAY) && !overflowed) {
    if (CC_SERIAL.available()) {
    
      #ifdef _STATUSLED
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on
      #endif
      
      while (CC_SERIAL.available()) {
        if (i == BUFFER_SIZE) {
          overflowed = true;
          DEBUG_PRINT(F("x"));
          break;
        }

        buffer[i] = (char)CC_SERIAL.read();
        i++;
        yield();
      }
      
      t_lastread = millis();
    }

    delay(10);
    yield();
  }

  CC_SERIAL.flush(); // Clear any remaining serial?

  /* If the buffer hasn't overflowed, process the message */
  if (!overflowed) {
    //DEBUG_PRINTLN(i); // See how big the payload is (166-288)
    
    /* Process the message */
    for (int j = 0; j < i; j++) {
      process_char(buffer[j]);
    }
    process_result();
  }

  /* Reset */
  i = 0;
  overflowed = false;  
}
