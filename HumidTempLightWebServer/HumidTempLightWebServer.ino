#include <dht11.h>
#include <Ethernet.h>

dht11 TEMHUMIDSENSOR;
#define TEMHUMIDSENSORPIN 8
#define LIGHTSENSORPIN 9

byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0x01, 0x02
};

EthernetServer server(80);
long lastReadingTime = 0;

double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

float humidity = 0.0;
float temperature = 0.0;
double dewpoint = 0.0;
int light = 0;

void getData()
{
  printTempHumidity();

  humidity = (float)TEMHUMIDSENSOR.humidity;
  temperature = (float)TEMHUMIDSENSOR.temperature;
  dewpoint = dewPointFast(temperature, humidity);

  light = digitalRead(LIGHTSENSORPIN);
}

void printTempHumidity()
{
  TEMHUMIDSENSOR.read(TEMHUMIDSENSORPIN);

  Serial.print("Humidity (%): ");
  Serial.println((float)TEMHUMIDSENSOR.humidity, 2);

  Serial.print("Temperature (°C): ");
  Serial.println((float)TEMHUMIDSENSOR.temperature, 2);

  Serial.print("Dew PointFast (°C): ");
  Serial.println(dewPointFast(TEMHUMIDSENSOR.temperature, TEMHUMIDSENSOR.humidity));

  Serial.println("");
}

void setup()
{
  Serial.begin(115200);
  while (!Serial); // wait for serial port to connect. Needed for native USB port only
  pinMode(LIGHTSENSORPIN, INPUT);

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  server.begin();
  delay(1000);
}


void loop()
{
  if (millis() - lastReadingTime > 1000) {
    switch (Ethernet.maintain()) {
      case 1:
        //renewed fail
        Serial.println("Error: renewed fail");
        break;

      case 2:
        //renewed success
        Serial.println("Renewed success");
        //print your local IP address:
        Serial.print("My IP address: ");
        Serial.println(Ethernet.localIP());
        break;

      case 3:
        //rebind fail
        Serial.println("Error: rebind fail");
        break;

      case 4:
        //rebind success
        Serial.println("Rebind success");
        //print your local IP address:
        Serial.print("My IP address: ");
        Serial.println(Ethernet.localIP());
        break;

      default:
        //nothing happened
        break;
    }
    getData();
    // timestamp the last time you got a reading:
    lastReadingTime = millis();

  }

  listenForEthernetClients();
}

void listenForEthernetClients() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          // print the current readings, in HTML format:
          client.print("Temperature: ");
          client.print(temperature);
          client.print(" degrees C");
          client.println("<br />");
          client.print("Humidity: " + String(humidity));
          client.print(" %");
          client.println("<br />");
          client.print("Dew point: " + String(dewpoint));
          client.print(" degrees C");
          client.println("<br />");
          client.print("Light sensor: ");
          client.print(light == 0 ? "on" : "off");
          client.println("<br />");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}
