#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <ESP8266mDNS.h>

char host_name[] = "motctrl";

const unsigned int cmdPort = 4760;
const unsigned int udnsPortIn = 4761;
const unsigned int udnsPortOut = 4762;
WiFiUDP Udp;
WiFiServer Tcp(cmdPort);
IPAddress multicast_group(224, 0, 0, 100);

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
	temp_out = 42;
	light = 76000;
	return 0;
}

void open_blind() {
	Serial.println("blind opened!");
}
void close_blind() {
	Serial.println("blind closed!");
}

void write(WiFiClient& client, int32_t value)
{
	const char *pdata = (const char*)&value;
	client.write(pdata, 4);
}

void announce(char *hostname, const IPAddress& addr)
{
  Serial.printf("# - announce: %s -> ", hostname); Serial.println(addr);
	WiFiUDP udp;
	//udp.beginPacket(IPAddress(255,255,255,255), localPortOut);
	udp.beginPacketMulticast(multicast_group, udnsPortOut, addr);
	udp.write('#');                                   // 1 byte header (#)
	uint32_t ip = addr;
	udp.write((uint8_t*)&ip, 4);                      // 4 bytes IP address
	udp.write(strlen(hostname));                      // 1 byte length
	udp.write(hostname);                              // N byte host name
	udp.endPacket();
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);

	WiFi.hostname(host_name);
	WiFiManager wifiManager;
	wifiManager.autoConnect("Akutron", "877D4754A1");

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
	Udp.beginMulticast(WiFi.localIP(), multicast_group, udnsPortIn);
	Tcp.begin();
	Serial.println("Motor controller started");
	announce(host_name, WiFi.localIP());
}

void loop() {
	// put your main code here, to run repeatedly:
	// if there's data available, read a packet
	if(Udp.parsePacket()) {
		// read the packet into packetBufffer
		char cmd = Udp.read();
    Udp.flush();
		switch(cmd) {
		case '$':
			Serial.print("$ - uDNS query, remote IP: "); Serial.println(Udp.remoteIP());
			announce(host_name, WiFi.localIP());
			break;
		}
	}

	WiFiClient client = Tcp.available();
	cmd_buf response;
	if(client) {
		while(!client.available())  delay(1);
		char cmd = client.read();
		client.flush();
		switch(cmd) {
		case 's': 
			response.status = update_sensors();
			response.light = light;
			response.temp = temp_out;
			Serial.printf("s - status: %02x, temp: %d, light: %d\n", response.status, (int)temp_out, light);
			//Serial.print(".");
			client.write((uint8_t*)response.data, sizeof(response));
      break;
		case 'o': open_blind(); break;
		case 'c': close_blind(); break;
		default:  Serial.printf("%c (%d) - unknown command\n", cmd, (int)cmd);
		}
   while(client.connected()) delay(1);
   client.stop();
	}

  long  fh = ESP.getFreeHeap();
  Serial.printf(": %d\n", (int)fh);
}

