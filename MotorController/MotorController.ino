#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <VEML7700.h>
#include "tmp75.h"

#define LED_ESP   2
#define LED_RED   0
#define LED_BLUE  16

const char version[]   = "ESP2.00";

const unsigned int udpPort = 4760;
IPAddress multicast_group(224, 0, 0, 100);

VEML7700 light_sensor;
Tmp75    temp_sensor(0x48, Tmp75::res12bit);
Ticker  red_blinker;

int32_t	light = 0;
int8_t	temp_out = 0;

void update_sensors() { 
  temp_out = temp_sensor.get_temp();

  float light_f = 0;
  light_sensor.getAutoALSLux(light_f);
  light = int32_t(light_f + 0.5);
  
  Serial.printf("Temp = %d, Light = %d\n", (int)temp_out, (int)light);
}

void write_status()
{
	WiFiUDP udp;
	udp.beginPacketMulticast(multicast_group, udpPort, WiFi.localIP());
	udp.write('s');                                   // 1 byte header (s)
  udp.write('\5');                                  // 1 byte size
  udp.write((uint8_t*)&temp_out, 1);                // 1 byte temperature
	udp.write((uint8_t*)&light, 4);                   // 4 bytes light
	udp.endPacket();
}

void on_wifi_event(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      Serial.println("WiFi lost connection");
      WiFi.disconnect(true);
      digitalWrite(LED_RED, LOW);
      delay(1000);
      ESP.reset();
      break;
    }
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
  Serial.println("");
  Serial.println("Motor controller setup");
  pinMode(LED_ESP,  OUTPUT); 
  pinMode(LED_RED,  OUTPUT); 
  pinMode(LED_BLUE, OUTPUT); 

  light_sensor.begin();
  temp_sensor.begin();
  red_blinker.attach_ms(100, []() { digitalWrite(LED_RED, !digitalRead(LED_RED)); });
  
/*	WiFi.hostname(host_name);
  //WiFi.setAutoReconnect(true);
  //WiFi.onEvent(on_wifi_event);
  
	WiFiManager wifiManager;
  //wifiManager.setTimeout(180);
  if(!wifiManager.autoConnect("Motctrl_AP")) {
    delay(3000);
    ESP.reset();
    delay(5000); 
	}

	//WiFi.begin("Akutron", "877D4754A1");
	//while (WiFi.status() != WL_CONNECTED) {
	//delay(500);
	//Serial.print(".");
	//}  

	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(WiFi.SSID());              // Tell us what network we're connected to
	Serial.print("IP address:\t");
	Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer
  Serial.print("Multicast group:\t");
  Serial.println(multicast_group);           // IP address for the name discovery

  red_blinker.detach();
  digitalWrite(LED_RED, HIGH);

	udns.beginMulticast(WiFi.localIP(), multicast_group, udnsPortIn);
	server.begin();
	server.setNoDelay(true);
  
	announce(host_name, WiFi.localIP());*/
  Serial.printf("Motor controller %s started\n", version);
  red_blinker.detach();
  digitalWrite(LED_ESP, HIGH);
  digitalWrite(LED_RED, HIGH);
}

void loop() {
  digitalWrite(LED_BLUE, LOW);
  update_sensors();
  delay(200);
  digitalWrite(LED_BLUE, HIGH);
  //write_status();
  delay(4000);
}

