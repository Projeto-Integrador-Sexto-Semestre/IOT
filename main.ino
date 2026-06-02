#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <WebServer.h>

// ===== WiFi =====
const char* WIFI_SSID = "GARCIA_2.G";
const char* WIFI_PASS = "Garcia141819";

// ===== DHT11 =====
#define DHTPIN    15
#define DHTTYPE   DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== LDR =====
#define LDRPIN        35
int LDR_THRESHOLD = 1500;

// ===== PIR =====
#define PIRPIN        14

// ===== Relé 1: Ventilador =====
#define RELAY_BLUE     16
#define LED_BLUE_PIN   26

// ===== Relé 2: Luz =====
#define RELAY_WHITE    17
#define LED_WHITE_PIN  26

// ===== Flags de controle =====
bool bloquearSensorLuz      = false;
bool bloquearSensorTemp     = false;
bool estadoManualVentilador = false;

float TEMP_THRESHOLD = 28.0;

WebServer          server(80);
WiFiClientSecure   wifiClient;
PubSubClient       mqttClient(wifiClient);

const char* MQTT_BROKER = "c5d8a79149ab41d89acfe6fd0306a95b.s1.eu.hivemq.cloud";
const uint16_t MQTT_PORT = 8883;
const char* MQTT_USER   = "Broker";
const char* MQTT_PASS   = "Fatec2026";

const char* TOPIC_TEMP         = "casa/sensor/temperatura";
const char* TOPIC_LDR          = "casa/sensor/luz";
const char* TOPIC_PIR          = "casa/sensor/presenca";
const char* TOPIC_COMMAND_VENT = "casa/command/ventilador";
const char* TOPIC_COMMAND_LED  = "casa/command/led_branco";
const char* TOPIC_COMMAND_TEMP = "casa/command/threshold/temp";

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String cmd;
  for (unsigned int i = 0; i < length; i++) cmd += (char)payload[i];
  String topicStr = String(topic);

  if (topicStr == TOPIC_COMMAND_VENT) {
    estadoManualVentilador = (cmd == "ON");
  } else if (topicStr == TOPIC_COMMAND_TEMP) {
    float val = cmd.toFloat();
    if (val > 0) TEMP_THRESHOLD = val;
  } else if (topicStr == TOPIC_COMMAND_LED) {
    bool on = (cmd == "ON");
    digitalWrite(RELAY_WHITE,   on ? LOW  : HIGH);
    digitalWrite(LED_WHITE_PIN, on ? HIGH : LOW);
  } else if (topicStr == "casa/command/threshold/ldr") {
    int val = cmd.toInt();
    if (val > 0) LDR_THRESHOLD = val;
  } else if (topicStr == "casa/command/bloqueio_sensor_luz") {
    bloquearSensorLuz = (cmd == "ON");
  } else if (topicStr == "casa/command/bloqueio_sensor_temp") {
    bloquearSensorTemp = (cmd == "ON");
  }
}

void mqttReconnect() {
  int tentativas = 0;
  while (!mqttClient.connected() && tentativas < 5) {
    Serial.print("Conectando MQTT...");
    if (mqttClient.connect("ESP32_Kit_LAFVIN", MQTT_USER, MQTT_PASS)) {
      Serial.println(" conectado!");
      mqttClient.subscribe(TOPIC_COMMAND_VENT);
      mqttClient.subscribe(TOPIC_COMMAND_LED);
      mqttClient.subscribe(TOPIC_COMMAND_TEMP);
      mqttClient.subscribe("casa/command/threshold/ldr");
      mqttClient.subscribe("casa/command/bloqueio_sensor_luz");
      mqttClient.subscribe("casa/command/bloqueio_sensor_temp");
    } else {
      Serial.printf(" falhou (rc=%d), tentando em 5s...\n", mqttClient.state());
      delay(5000);
      tentativas++;
    }
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado: " + WiFi.localIP().toString());

  wifiClient.setInsecure();

  dht.begin();
  pinMode(LDRPIN, INPUT);
  pinMode(PIRPIN, INPUT);

  pinMode(RELAY_BLUE,    OUTPUT); digitalWrite(RELAY_BLUE,    HIGH);
  pinMode(LED_BLUE_PIN,  OUTPUT); digitalWrite(LED_BLUE_PIN,  LOW);
  pinMode(RELAY_WHITE,   OUTPUT); digitalWrite(RELAY_WHITE,   HIGH);
  pinMode(LED_WHITE_PIN, OUTPUT); digitalWrite(LED_WHITE_PIN, LOW);

  server.on("/toggle-luz", []() {
    static bool st = false;
    st = !st;
    digitalWrite(RELAY_WHITE,   st ? LOW  : HIGH);
    digitalWrite(LED_WHITE_PIN, st ? HIGH : LOW);
    server.send(200, "text/plain", st ? "Luz ON" : "Luz OFF");
  });

  server.on("/set-threshold", []() {
    if (server.hasArg("value")) {
      TEMP_THRESHOLD = server.arg("value").toFloat();
      server.send(200, "text/plain", "Threshold=" + String(TEMP_THRESHOLD));
    } else {
      server.send(400, "text/plain", "Parametro faltando");
    }
  });

  server.begin();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

unsigned long lastPublish = 0;

void loop() {
  server.handleClient();
  if (!mqttClient.connected()) mqttReconnect();
  mqttClient.loop();

  if (millis() - lastPublish >= 10000) {
    lastPublish = millis();

    float temp = dht.readTemperature();
    int   ldr  = analogRead(LDRPIN);
    bool  pres = digitalRead(PIRPIN) == HIGH;

    if (!isnan(temp)) mqttClient.publish(TOPIC_TEMP, String(temp).c_str(), true);
    mqttClient.publish(TOPIC_LDR, (ldr < LDR_THRESHOLD ? "DARK" : "BRIGHT"), true);
    mqttClient.publish(TOPIC_PIR, pres ? "1" : "0", true);

    if (!bloquearSensorLuz) {
      bool acender = (ldr < LDR_THRESHOLD && pres);
      digitalWrite(RELAY_WHITE,   acender ? LOW  : HIGH);
      digitalWrite(LED_WHITE_PIN, acender ? HIGH : LOW);
    }

    if (!bloquearSensorTemp) {
      bool ligar = (!isnan(temp) && temp > TEMP_THRESHOLD);
      digitalWrite(RELAY_BLUE,   ligar ? LOW  : HIGH);
      digitalWrite(LED_BLUE_PIN, ligar ? HIGH : LOW);
    } else {
      digitalWrite(RELAY_BLUE,   estadoManualVentilador ? LOW  : HIGH);
      digitalWrite(LED_BLUE_PIN, estadoManualVentilador ? HIGH : LOW);
    }

    Serial.printf("Temp: %.1f C (limite: %.1f C) | LDR: %d | Presenca: %s\n",
                  temp, TEMP_THRESHOLD, ldr, pres ? "SIM" : "NAO");
  }

  delay(100);
}
