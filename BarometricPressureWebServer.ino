/*
 BMP180 Barometric Pressure Sensor Display and Web Server
 Serves the output of a Barometric Pressure Sensor as a web page.
 Circuit: BMP180 sensor + Display 1602a1 connected to I2C
 created 2018-05-10 Joachim Schwender
 */
#include <Wire.h>
#include <BMP180.h>
#include <Ethernet2.h>  // Arduino Leonardo ETH mit W5500 Chip
#include <LiquidCrystal_I2C.h>// Vorher hinzugefügte LiquidCrystal_I2C Bibliothek hochladen

LiquidCrystal_I2C lcd(0x3F, 16, 2);   //Hier wird festgelegt um was für einen Display es sich handelt. In diesem Fall einer mit 16 Zeichen in 2 Zeilen.
BMP180 barometer; // MP180 Sensor;
byte mac[] = {  0x90, 0xA2, 0xDA, 0x10, 0xFC, 0xDA };  // adjust to the MAC addreess of your device
IPAddress ip(192, 168, 178, 33);  // assign a MAC address for the Ethernet controller.
byte subnet[] = { 255, 255, 255, 0 };
byte gateway[] = { 192, 168, 178, 1 };

// Initialize the Ethernet server library with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

float AnfangsDruck;
long lastReadingTime = 0;

void setup() {
  barometer.init();
  barometer.setSamplingMode(BMP180_OVERSAMPLING_ULTRA_HIGH_RESOLUTION);
  Serial.begin(9600);
  if (!barometer.hasValidID())
    Serial.println("Error - please check the BMP180 board!");
  lcd.init(); //Im Setup wird der LCD gestartet (anders als beim einfachen LCD Modul ohne 16,2 in den Klammern denn das wurde vorher festgelegt
  lcd.backlight(); // Hintergrundlicht einschalten
  lcd.setCursor(0,0);  // erste Zeile, Anfang
  lcd.print("BMP180 Sensor Web");
  lcd.setCursor(0,1);  // zweite Zeile, Anfang
  lcd.print("J. Schwender");
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  barometer.setP0();
  barometer.getData();
  AnfangsDruck = barometer.P;
  // give the sensor and Ethernet shield time to set up:
  delay(1000);
  lcd.clear();
}

void loop() {
  // check for a reading no more than once a second.
  if (millis() - lastReadingTime > 1000) {
    // if there's a reading ready, read it:
      getData();
      // timestamp the last time you got a reading:
      lastReadingTime = millis();
  }
  // listen for incoming Ethernet connections:
  listenForEthernetClients();
}


void getData() {
  Serial.println("Getting reading");
  //Read the temperature data
  barometer.getData();
  float Pdiff = barometer.P - AnfangsDruck;
  float h = barometer.getAltitude();
  lcd.setCursor(0,0);
  lcd.print(String(barometer.T) + " \337C, " + String(Pdiff));
  lcd.setCursor(0,1);
  lcd.print(String(barometer.P) + " hPa");
  Serial.println("Temperature: " + String(barometer.T) + " °C, Pressure: " + String(barometer.P) + " hPa");
}

void listenForEthernetClients() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("   Got a http request");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
//    Serial.print("   read  ");
        char c = client.read();
    Serial.print(c);
          // send a standard http response header
    Serial.println("   send a reply now");
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html><H1>Arduino Leonardo ETH mit BMP 180</H1>Temperature: " + String(barometer.T) + " &deg;C<br>Pressure: " + String(barometer.P) + " hPa<br></html>");
          break;
     }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}




