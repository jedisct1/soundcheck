#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

/* Delay between each sample, in ms */
#define DELAY 100

/* How many samples to consider for the moving average */
#define WINDOW 1000

/* Threshold for the sound level to be considered suspicious */
#define THRESHOLD 2.0

/* How many suspicious samples to consider before triggering an alert */
#define THRESHOLD_ROUNDS_BEFORE_ALERT 100

/* (time to recover / time for alert) ratio */
#define THRESHOLD_DECREASE_RATIO 4

/* WiFi SSID */
const char *wifi_ssid = "XXX"; // *** CHANGE THIS ***

/* WiFi password */
const char *wifi_pass = "XXX"; // *** CHANGE THIS ***

/* Server IP to connect to */
IPAddress server(127, 0, 0, 1); // *** CHANGE THIS ***

/* HTTP server hostname */
const char *host_name = "sms.example.org"; // *** CHANGE THIS ***

/* Digital pin the mic is connect to */
const int sensor_pin = A0;

double ewma = 0.0;
double ewmavar = 0.0;
unsigned int threshold_counter = 0;
bool alert_sent = false;
int wifi_status = WL_IDLE_STATUS;
WiFiClient client;

#define DECAY (2.0 / ((double) WINDOW + 1.0))

static void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

static void connect_to_wifi() {
  while (wifi_status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(wifi_ssid);
    wifi_status = WiFi.begin((char *) wifi_ssid, (char *) wifi_pass);
    delay(10000);
  }
  Serial.println("Connected to wifi");
  printWifiStatus();
  delay(5000);
}

static void disconnect_from_wifi() {
  WiFi.disconnect();
  wifi_status = WL_IDLE_STATUS;
  delay(10000);
  Serial.println("Disconnected from wifi");
}

void setup() {
  Serial.begin (9600);
  while (!Serial);
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
  }
  Serial.println("Probe is active");
}

static void send_sms(const char *msg, double sensor_value, double ewma) {
  connect_to_wifi();
  if (client.connect(server, 80)) {
    Serial.println("connected to server");
    client.println("GET /send HTTP/1.1");
    client.println("Host: " + String(host_name));
    client.println("Message: " + String(msg) + " (sensor " + (unsigned) sensor_value + " ewma " + (unsigned) ewma + ")");
    client.println("Connection: close");
    client.println("");
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
    delay(5000);
    client.stop();
  }
  disconnect_from_wifi();
}

void loop() {
  int sensor_value = analogRead (sensor_pin);
  if (ewma == 0.0) {
    ewma = sensor_value;
    ewmavar = 0.0;
    send_sms("Probe is active", sensor_value, ewma);
  } else {
    ewma = (sensor_value * DECAY) + (ewma * (1.0 - DECAY));
    double var = abs(sensor_value - ewma);
    ewmavar = (var * DECAY) + (ewmavar * (1.0 - DECAY));
  }
  if (sensor_value - ewma > ewmavar * THRESHOLD) {
    if (alert_sent == false && threshold_counter++ == THRESHOLD_ROUNDS_BEFORE_ALERT) {
      send_sms("[WARNING] Sound level", sensor_value, ewma);
      alert_sent = true;
      threshold_counter *= THRESHOLD_DECREASE_RATIO;
    }
  } else if (alert_sent != false && threshold_counter > 0) {
    if (--threshold_counter == 0) {
      send_sms("[NORMAL] Sound level back to normal", sensor_value, ewma);
      alert_sent = false;
    }
  }
  delay (DELAY);
}
