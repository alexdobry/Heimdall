#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const int mqtt_port = 1883;
const char* client_name = "doorbell";

const char* state = "state/doorbell";
const char* ring = "state/doorbell/ring";
const char* sound = "state/doorbell/sound";
const char* commands = "commands/doorbell";

WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;

bool alreadyTriggered = false;
const int doorBellPin = 16; // D0
const int silencePin = 2;  // D4
bool boot = true;

void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  int retries = 0;
  
  while (!client.connected()) {
    if(retries < 15) {
      Serial.print("Attempting MQTT connection...");
      
      if (client.connect(client_name)) {
        Serial.println("connected");

        if(boot == true) {
          client.publish(state,"Rebooted");
          boot = false;
        }
        
        if(boot == false) {
          client.publish(state,"Reconnected"); 
        }
        
        client.subscribe(commands);
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        delay(5000); // Wait 5 seconds before retrying
      }
    }
    
    if(retries > 14) {
      Serial.println("Restarting ESP");
      ESP.restart();
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  
  String message = String((char *)payload);
  Serial.println(message);
  Serial.println();
  
  if (newTopic == commands) {
    if (message == "off") {
      digitalWrite(silencePin, LOW);
      client.publish(sound, "off", true);
    }
    
    if (message == "on") {
      digitalWrite(silencePin, HIGH);
      client.publish(sound, "on", true);
    }
  }
}

void getDoorBell() {
  if(digitalRead(doorBellPin) == 1 && alreadyTriggered == false) {
    client.publish(ring, "on");
    alreadyTriggered = true;
    timer.setTimeout(6000, resetTrigger); 
  }
}

void resetTrigger() {
  alreadyTriggered = false;
}

void ping() {
  client.publish(state, "ok");
}

void setup() {
  Serial.begin(115200);

  pinMode(doorBellPin, INPUT_PULLDOWN_16);
  pinMode(silencePin, OUTPUT);
  
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  ArduinoOTA.setHostname(client_name);
  ArduinoOTA.begin(); 
  
  timer.setInterval(120000, ping);
  timer.setInterval(200, getDoorBell);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();
  ArduinoOTA.handle();
  timer.run();
}
