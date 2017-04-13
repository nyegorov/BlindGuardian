#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <ESP8266mDNS.h>

#define MAX_SRV_CLIENTS 5

char host_name[] = "motctrl";

const unsigned int cmdPort = 4760;
const unsigned int udnsPortIn = 4761;
const unsigned int udnsPortOut = 4762;
WiFiUDP udns;
IPAddress multicast_group(224, 0, 0, 100);

WiFiServer server(cmdPort);
WiFiClient clients[MAX_SRV_CLIENTS];

int32_t	light = 0;
int8_t	temp_out = 0;

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
	return 0;
}

void open_blind() {
	Serial.println("blind opened!");
}
void close_blind() {
	Serial.println("blind closed!");
}

void announce(char *hostname, IPAddress addr)
{
  Serial.printf("# - announce: %s -> ", hostname); Serial.println(addr);
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
    case WIFI_EVENT_STAMODE_GOT_IP: break;
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

	//if(!MDNS.begin(host_name)) 		Serial.println("Error setting up MDNS responder!");

	//Udp.begin(localPortIn);
	udns.beginMulticast(WiFi.localIP(), multicast_group, udnsPortIn);
  
	server.begin();
	server.setNoDelay(true);
  
	announce(host_name, WiFi.localIP());
  Serial.println("Motor controller started");
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
			Serial.print("$ - uDNS query, remote IP: "); Serial.println(udns.remoteIP());
			announce(host_name, WiFi.localIP());
			break;
    case 'r': ESP.reset(); break;
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
        cmd_buf response;
        char cmd = clients[i].read();
        clients[i].flush();
        switch(cmd) {
        case 's': 
          response.status = update_sensors();
          response.light = light;
          response.temp = temp_out;
          Serial.printf("s - status: %02x, temp: %d, light: %d, mem: %u\n", response.status, (int)temp_out, light, (unsigned)ESP.getFreeHeap());
          //Serial.print(".");
          clients[i].write((uint8_t*)response.data, sizeof(response));
          break;
        case 'o': open_blind(); break;
        case 'c': close_blind(); break;
        case 'r': ESP.reset(); break;
        default:  Serial.printf("%c (%d) - unknown command\n", cmd, (int)cmd);
        }
      }
    }
  }
  //long  fh = ESP.getFreeHeap();
  //Serial.printf(": %d\n", (int)fh);
}

