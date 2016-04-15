#include <ESP8266WiFi.h>
#include <PubSubClient.h>     // using the library from https://github.com/Imroy/pubsubclient
#include <DHT.h>              // using the library "DHT sensor library by Adafruit"

/*  Begin of Constants  */
      /* WIFI Settings */
      const char *ssid =      "My SSID";
      const char *password =  "My Password";
      
      /* Azure IOT Hub Settings */
      const char* mqtt_server = "kevinsay.azure-devices.net";
      const char* devicename =  "one";
      //            Example     "SharedAccessSignature sr=kevinsay.azure-devices.net%2fdevices%2fone&sig=UAWhZbahfeVd13%2bJRDh2FbsDhZeye7RQYm6qxKlyP48%3d&se=1491699042";
      const char* devicesas =   "{ Generate this from Device Explorer }";
      long interval =           60000; //(ms) - 60 seconds between reports
/*   End of Constants   */

#define DHTTYPE DHT11 // DHT11 or DHT22 
#define DHTPIN  D4    // On the ESP8266, we are using D4 as the pin location

DHT dht(DHTPIN, DHTTYPE, 11);
float h, t;

WiFiClientSecure espClient;
PubSubClient client(espClient, mqtt_server, 8883);
void callback(const MQTT::Publish& pub);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.println("Connecting to MQTT server");
      String  deviceid=mqtt_server;
              deviceid+="/";
              deviceid+=devicename;
      
      Serial.print("DeviceID: ");
      Serial.println(deviceid);
      
      if (client.connect(MQTT::Connect(devicename).set_auth(deviceid, devicesas))) {
        String subscribestring="devices/";
        subscribestring+=devicename;
        subscribestring+="/messages/devicebound/#";

        Serial.println("Connected to MQTT server");

        client.subscribe(subscribestring);
        Serial.print("Subscribing to: ");
        Serial.println(subscribestring);
      }
      else {
        Serial.println("Could not connect to MQTT server");
      }
    }

    if (client.connected())
    {
      Serial.println("client.connected()");
       
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
                
      h = dht.readHumidity();
      t = dht.readTemperature(true);    // True means Farenheight

      h = h * 1.23;
      t = t*1.1;

      // Check if any reads failed and exit early (to try again).
      if (isnan(h) || isnan(t)) {
          Serial.println("Failed to read from DHT sensor!");
        }
        else if (!client.connected()) {
          Serial.println("Connection to broker lost; retrying");
        }
        else {
          char* tPayload = f2s(t, 0);
          char* hPayload = f2s(h, 0);
          
          // We have to build a json object so stream analytics can understand it
          String  json="{ temp: '";
                  json+=tPayload;
                  json+="', humidity: '";
                  json+=hPayload;
                  json+="', deviceId: '";
                  json+=devicename;
                  json+="', ssid: '";
                  json+=ssid;
                  json+="' }";

          String publishstring="devices/";
          publishstring+=devicename;
          publishstring+="/messages/events/";

          client.publish(publishstring, json);

          Serial.print("published environmental data: ");
          Serial.print(json);
          Serial.print(" to ");
          Serial.println(publishstring);

          digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
        }
      } else {
        Serial.println("client.connected() = false");
      }
    }
  delay(interval);    
}

/* float to string
   f is the float to turn into a string
   p is the precision (number of decimals)
   return a string representation of the float.
*/
char *f2s(float f, int p) {
  char * pBuff;                         // use to remember which part of the buffer to use for dtostrf
  const int iSize = 10;                 // number of buffers, one for each float before wrapping around
  static char sBuff[iSize][20];         // space for 20 characters including NULL terminator for each float
  static int iCount = 0;                // keep a tab of next place in sBuff to use
  pBuff = sBuff[iCount];                // use this buffer
  if (iCount >= iSize - 1) {            // check for wrap
    iCount = 0;                         // if wrapping start again and reset
  }
  else {
    iCount++;                           // advance the counter
  }
  return dtostrf(f, 0, p, pBuff);       // call the library function
}
