#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <WiFiServer.h>
#include <ESP8266mDNS.h>

char host_name[] = "motctrl";

unsigned int cmdPort = 4760;
unsigned int udnsPortIn = 4761;
unsigned int udnsPortOut = 4762;
WiFiUDP Udp;
WiFiServer Tcp(cmdPort);
IPAddress multicast_group(239, 255, 1, 2);

int32_t get_temp() { 
	int32_t temp = 42;
	Serial.printf("ask temp (%d)\n", (int)temp);
	return temp;
}
int32_t get_light() {
	int32_t light = 76000;
	Serial.printf("ask light (%d)\n", (int)light);
	return light;
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
		switch(cmd) {
		case '$':
			Serial.print("$ - uDNS query, remote IP: "); Serial.println(Udp.remoteIP());
			announce(host_name, WiFi.localIP());
			break;
		}
		Udp.flush();
	}

	WiFiClient client = Tcp.available();
	if(client) {
		while(!client.available())  delay(1);
		char cmd = client.read();
		client.flush();
		switch(cmd) {
		case 't': write(client, get_temp()); break;
		case 'l': write(client, get_light()); break;
		case 'o': open_blind(); break;
		case 'c': close_blind(); break;
		default:  Serial.printf("%c (%d) - unknown command", cmd, (int)cmd);
		}
	}
}

