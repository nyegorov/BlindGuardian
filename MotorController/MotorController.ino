#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <VEML7700.h>
#include "tmp75.h"

#define MAX_SRV_CLIENTS 5
#define LED_RED   3
#define LED_GREEN 4

char version[]   = "ESP1.03";
char host_name[] = "motctrl";

const unsigned int cmdPort = 4760;
const unsigned int udnsPortIn = 4761;
const unsigned int udnsPortOut = 4762;
WiFiUDP udns;
IPAddress multicast_group(224, 0, 0, 100);

VEML7700 light_sensor;
Tmp75    temp_sensor(0x4F, Tmp75::res12bit);
Ticker  red_blinker;
WiFiServer server(cmdPort);
WiFiClient clients[MAX_SRV_CLIENTS];

bool red_light = false;
int32_t	light = 0;
int8_t	temp_out = 0;
uint8_t position = 0;

#pragma pack(push, 1)
union cmd_buf {
	struct {
		uint8_t status;
		uint8_t temp;
		uint32_t light;
	};
	uint8_t data[6];
};
#pragma pack(pop)

uint8_t update_sensors() { 
	temp_out = 42 + random(5);
	light = 76000 + random(1000);
	return position;
}

void write_status(WiFiClient& client) {
  cmd_buf response;
  response.status = position;
  response.light = light;
  response.temp = temp_out;
  Serial.printf("s -> pos: %d, temp: %d, light: %d, mem: %u\n", response.status, (int)temp_out, light, (unsigned)ESP.getFreeHeap());
  //Serial.print(".");
  client.write((uint8_t*)response.data, sizeof(response));
}

void set_pos(uint8_t pos) {
  position = pos;
  if(position == 100)       Serial.println("blind opened!");
  else if(position ==   0)  Serial.println("blind closed!");
  else                    	Serial.printf("blind set to %d\n", (int)position);
}

void announce(char *hostname, IPAddress addr)
{
  Serial.printf("# -> %s: ", hostname); Serial.println(addr);
	WiFiUDP udp;
	udp.beginPacketMulticast(multicast_group, udnsPortOut, addr);
	udp.write('#');                                   // 1 byte header (#)
	uint32_t ip = addr;
	udp.write((uint8_t*)&ip, 4);                      // 4 bytes IP address
	udp.write(strlen(hostname));                      // 1 byte length
	udp.write(hostname);                              // N byte host name
	udp.endPacket();
}

void on_wifi_event(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      Serial.println("WiFi lost connection");
      WiFi.disconnect(true);
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

  light_sensor.begin();
  temp_sensor.begin();
  pinMode(LED_RED, OUTPUT); 
  pinMode(LED_GREEN, OUTPUT); 
  red_blinker.attach(0.5, []() { digitalWrite(LED_RED, red_light ? HIGH : LOW); red_light = !red_light; });
  
	WiFi.hostname(host_name);
  //WiFi.setAutoReconnect(true);
  //WiFi.onEvent(on_wifi_event);
  
	WiFiManager wifiManager;
  //wifiManager.setTimeout(180);
  if(!wifiManager.autoConnect("Motctrl_AP")) {
    delay(3000);
    ESP.reset();
    delay(5000); 
	}

	/*  WiFi.begin("Akutron", "877D4754A1");
	while (WiFi.status() != WL_CONNECTED) {
	delay(500);
	Serial.print(".");
	}  */

	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(WiFi.SSID());              // Tell us what network we're connected to
	Serial.print("IP address:\t");
	Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer*/
  Serial.print("Multicast group:\t");
  Serial.println(multicast_group);           // IP address for the name discovery*/

  red_blinker.detach();
  digitalWrite(LED_RED, HIGH);

	udns.beginMulticast(WiFi.localIP(), multicast_group, udnsPortIn);
	server.begin();
	server.setNoDelay(true);
  
	announce(host_name, WiFi.localIP());
  Serial.printf("Motor controller %s started\n", version);
}

void loop() {
  int i;
  
	// if there's data available, read a packet
	if(udns.parsePacket()) {
		// read the packet into packetBufffer
		char cmd = udns.read();
    udns.flush();
		switch(cmd) {
		case '$':
			Serial.print("$ <- uDNS query, remote IP: "); Serial.println(udns.remoteIP());
			announce(host_name, WiFi.localIP());
			break;
    case 'r': 
      WiFi.disconnect(true);
      delay(1000);
      ESP.reset(); 
      break;
		}
	}

  if (server.hasClient()){
    bool accepted = false;
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!clients[i] || !clients[i].connected()){
        if(clients[i]) {
          Serial.println("disconnect client");
          clients[i].stop();
        }
        clients[i] = server.available();
        if(clients[i].connected()) {
          Serial.print("connected client: "); Serial.println(clients[i].remoteIP());
          accepted = true;
        }
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient client = server.available();
    client.stop();
    if(!accepted) {
      // cann't accept connection, perhaps socket hangs
      Serial.println("reject connection");
      ESP.reset();
    }
  }
  //check clients for data
  for(i = 0; i < MAX_SRV_CLIENTS; i++){
    if (clients[i] && clients[i].connected()){
      if(clients[i].available()){
        char cmd = clients[i].read();
        switch(cmd) {
        case 's': update_sensors();           write_status(clients[i]); break;
        case 'o': set_pos(100);               write_status(clients[i]); break;
        case 'p': set_pos(clients[i].read()); write_status(clients[i]); break;
        case 'c': set_pos(0);                 write_status(clients[i]); break;
        case 'v': Serial.printf("v -> %s\n", version); clients[i].write((uint8_t*)version, 8); break;
        case 'r': ESP.reset(); break;
        default:  Serial.printf("%c <- unknown command (%d)\n", cmd, (int)cmd);
        }
      }
    }
  }
}

