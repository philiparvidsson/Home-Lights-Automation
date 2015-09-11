#include <Ethernet.h>
#include <SPI.h>

#define AuthString               ("<removed>") // username:password in base64
#define RouterAttachedDevicesURL ("/DEV_show_device.htm")
#define DeviceConnected          (1)
#define DeviceDisconnected       (2)
#define Unknown                  (0)

static const uint8_t router_ip[] = { 192, 168, 1, 1 };
static const uint8_t mac_addr [] = { 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed };

int isDeviceOnLAN(const char* device_mac_addr) {
  EthernetClient* client = new EthernetClient();

  if (!client->connect(router_ip, 80)) {
    Serial.println("ERROR: Could not connect to router.");
    return (Unknown);
  }

  client->print("GET ");
  client->print(RouterAttachedDevicesURL);
  client->println(" HTTP/1.1");
  client->print("Authorization: Basic ");
  client->println(AuthString);
  client->println("Connection: close");
  client->println("");
  client->println("");
  client->flush();

  Serial.println("Request sent....");

  // Give the router a chance to respond to the request.
  delay(5000);

  if (!client->available()) {
    Serial.println("ERROR: No response received.");
    return (Unknown);
  }

  bool device_found = false;

  while (client->available()) {
    char c = client->read();

    if ((c == *device_mac_addr) && (!(*(++device_mac_addr)))) {
      delete client;
      return (DeviceConnected);
    }
  }

  delete client;
  return (DeviceDisconnected);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { /* ... */ }

  if (!Ethernet.begin((uint8_t*)mac_addr)) {
    Serial.println("Ethernet.begin() failed.");
    while (true) { /* ... */ }
  }

  delay(1000);
}

void loop() {
  Ethernet.maintain();

  int r = isDeviceOnLAN("f0:d1:a9:af:46:eb");

  if (r == DeviceConnected) {
    Serial.println("Phil is at home");
  }
  else if (r == DeviceDisconnected) {
    Serial.println("Phil is not at home");
  }
  else {
    Serial.println("No idea where Phil is");
  }
  delay(60000);
}

