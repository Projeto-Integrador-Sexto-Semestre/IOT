#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <WebServer.h>

#include <WiFiClientSecure.h>
#include <HTTPClient.h>


#define BOT_TOKEN "SEU_TOKEN_AQUI"
#define CHAT_ID_1 "SEU_CHAT_ID"
#define CHAT_ID_2 "CHAT_ID_AMIGO"

// ===== DHT22 =====
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ===== MQ2 =====
#define MQ2PIN        34
#define MQ2_THRESHOLD 2000

// ===== LDR =====
#define LDRPIN        35
int LDR_THRESHOLD = 1500;

// ===== PIR =====
#define PIRPIN        14

// ===== Relés e LEDs =====
#define RELAY_RED      16
#define LED_RED_PIN    25
#define RELAY_BLUE     17
#define LED_BLUE_PIN   26
#define RELAY_WHITE    18
#define LED_WHITE_PIN  27

// ===== Buzzer =====
#define BUZZER_PIN     13

// ===== Motor de passo =====
#define STEP_PIN       33
#define DIR_PIN        32

bool bloquearSensorLuz = false;
bool bloquearSensorTemp = false;
bool estadoManualVentilador = false;

void sendTelegram(const String& text) {
  WiFiClientSecure client;
  client.setInsecure(); // Ignora o certificado SSL
  HTTPClient https;

  // Lista de IDs
  String chatIDs[] = { CHAT_ID_1, CHAT_ID_2 };

  for (int i = 0; i < 2; i++) {
    String url = "https://api.telegram.org/bot" BOT_TOKEN
                 "/sendMessage?chat_id=" + chatIDs[i] + "&text=" + text;

    if (https.begin(client, url)) {
      https.GET();  // Envia a mensagem
      https.end();
    }
  }
}
// Limite térmico
float TEMP_THRESHOLD = 28.0;

// HTTP server
WebServer server(80);

// MQTT broker
const char* MQTT_BROKER = "test.mosquitto.org";
const uint16_t MQTT_PORT = 1883;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// MQTT Topics
const char* TOPIC_TEMP         = "casa/sensor/temperatura";
const char* TOPIC_GAS          = "casa/sensor/gas";
const char* TOPIC_LDR          = "casa/sensor/luz";
const char* TOPIC_PIR          = "casa/sensor/presenca";
const char* TOPIC_COMMAND_VENT = "casa/command/ventilador";
const char* TOPIC_COMMAND_LED  = "casa/command/led_branco";
const char* TOPIC_COMMAND_TEMP = "casa/command/threshold/temp";

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  String cmd;
  for (unsigned int i = 0; i < length; i++) cmd += (char)payload[i];

  Serial.print("Payload: ");
  Serial.println(cmd);

  String topicStr = String(topic);

  if (topicStr == TOPIC_COMMAND_VENT) {
    if (cmd == "ON") {
      estadoManualVentilador = true;
      Serial.println("✅ Ventilador LIGADO manualmente.");
    } else {
      estadoManualVentilador = false;
      Serial.println("✅ Ventilador DESLIGADO manualmente.");
    }
  }

  else if (topicStr == TOPIC_COMMAND_TEMP) {
    float val = cmd.toFloat();
    if (val > 0) {
      TEMP_THRESHOLD = val;
      Serial.print("Novo limiar de temperatura: ");
      Serial.println(TEMP_THRESHOLD);
    }
  }

  else if (topicStr == TOPIC_COMMAND_LED) {
    if (cmd == "ON") {
      digitalWrite(RELAY_WHITE, LOW);
      digitalWrite(LED_WHITE_PIN, HIGH);
    } else {
      digitalWrite(RELAY_WHITE, HIGH);
      digitalWrite(LED_WHITE_PIN, LOW);
    }
  }

  else if (topicStr == "casa/command/threshold/ldr") {
    int val = cmd.toInt();
    if (val > 0) {
      LDR_THRESHOLD = val;
      Serial.print("Novo limiar de luminosidade: ");
      Serial.println(LDR_THRESHOLD);
    }
  }

  else if (topicStr == "casa/command/bloqueio_sensor_luz") {
    if (cmd == "ON") {
      bloquearSensorLuz = true;
      Serial.println("🚫 Bloqueio de sensor de luz ATIVADO.");
    } else {
      bloquearSensorLuz = false;
      Serial.println("✅ Bloqueio de sensor de luz DESATIVADO.");
    }
  }

  else if (topicStr == "casa/command/bloqueio_sensor_temp") {
    if (cmd == "ON") {
      bloquearSensorTemp = true;
      Serial.println("🚫 Bloqueio de sensor de temperatura ATIVADO.");
    } else {
      bloquearSensorTemp = false;
      Serial.println("✅ Bloqueio de sensor de temperatura DESATIVADO.");
    }
  }
}

void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Tentando conectar MQTT... ");
    if (mqttClient.connect("ESP32_Simulacao")) {
      Serial.println("Conectado!");
      mqttClient.subscribe("casa/command/threshold/temp");
      mqttClient.subscribe("casa/command/threshold/ldr");
      mqttClient.subscribe("casa/command/bloqueio_sensor_luz");
      mqttClient.subscribe("casa/command/bloqueio_sensor_temp");
      mqttClient.subscribe(TOPIC_COMMAND_VENT);
      mqttClient.subscribe(TOPIC_COMMAND_LED);
      mqttClient.subscribe(TOPIC_COMMAND_TEMP);
    } else {
      Serial.print("Erro, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" tentando novamente em 5s...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado: " + WiFi.localIP().toString());

  dht.begin();
  pinMode(MQ2PIN, INPUT);
  pinMode(LDRPIN, INPUT);
  pinMode(PIRPIN, INPUT);

  pinMode(RELAY_RED, OUTPUT);     digitalWrite(RELAY_RED, HIGH);
  pinMode(LED_RED_PIN, OUTPUT);   digitalWrite(LED_RED_PIN, LOW);
  pinMode(RELAY_BLUE, OUTPUT);    digitalWrite(RELAY_BLUE, HIGH);
  pinMode(LED_BLUE_PIN, OUTPUT);  digitalWrite(LED_BLUE_PIN, LOW);
  pinMode(RELAY_WHITE, OUTPUT);   digitalWrite(RELAY_WHITE, HIGH);
  pinMode(LED_WHITE_PIN, OUTPUT); digitalWrite(LED_WHITE_PIN, LOW);

  pinMode(BUZZER_PIN, OUTPUT);    digitalWrite(BUZZER_PIN, LOW);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  server.on("/toggle-led", []() {
    static bool st = false;
    st = !st;
    digitalWrite(LED_RED_PIN, st ? HIGH : LOW);
    digitalWrite(RELAY_RED, st ? LOW : HIGH);
    server.send(200, "text/plain", st ? "LED Vermelho ON" : "OFF");
  });

  server.on("/set-threshold", []() {
    if (server.hasArg("value")) {
      TEMP_THRESHOLD = server.arg("value").toFloat();
      server.send(200,"text/plain","Threshold=" + String(TEMP_THRESHOLD));
    } else server.send(400,"text/plain","Parametro faltando");
  });

  server.begin();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  Serial.println("Setup completo.");
}

unsigned long lastPublish = 0;

void loop() {
  server.handleClient();

  if (!mqttClient.connected()) mqttReconnect();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublish >= 10000) {
    lastPublish = now;

    float temp = dht.readTemperature();
    int gas = analogRead(MQ2PIN);
    int ldr = analogRead(LDRPIN);
    bool pres = digitalRead(PIRPIN) == HIGH;

    if (!isnan(temp)) mqttClient.publish(TOPIC_TEMP, String(temp).c_str(), true);
    mqttClient.publish(TOPIC_GAS, (gas > MQ2_THRESHOLD ? "ALERT" : "OK"), true);
    mqttClient.publish(TOPIC_LDR, (ldr < LDR_THRESHOLD ? "DARK" : "BRIGHT"), true);
    mqttClient.publish(TOPIC_PIR, pres ? "1" : "0", true);

    // Gás
    if (gas > MQ2_THRESHOLD) {
      digitalWrite(RELAY_RED, LOW);
      digitalWrite(LED_RED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      sendTelegram("⚠️ ALERTA DE GÁS DETECTADO! ");
    } else {
      digitalWrite(RELAY_RED, HIGH);
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
    }

    // Luz automática
    if (!bloquearSensorLuz) {
      if (ldr < LDR_THRESHOLD && pres) {
        digitalWrite(RELAY_WHITE, LOW);
        digitalWrite(LED_WHITE_PIN, HIGH);
        sendTelegram("👤 Movimento detectado em ambiente escuro.");
      } else {
        digitalWrite(RELAY_WHITE, HIGH);
        digitalWrite(LED_WHITE_PIN, LOW);
      }
    } else {
      Serial.println("🚫 Iluminação automática BLOQUEADA pelo usuário.");
    }

    // Temperatura
    if (!bloquearSensorTemp) {
      if (!isnan(temp) && temp > TEMP_THRESHOLD) {
        digitalWrite(RELAY_BLUE, LOW);
        digitalWrite(LED_BLUE_PIN, HIGH);
        digitalWrite(DIR_PIN, HIGH);
        digitalWrite(STEP_PIN, HIGH); delay(2);
        digitalWrite(STEP_PIN, LOW);  delay(2);
        sendTelegram("🔥 Temperatura acima do limite! " + String(temp) + " °C");
      } else {
        digitalWrite(RELAY_BLUE, HIGH);
        digitalWrite(LED_BLUE_PIN, LOW);
      }
    } else {
      if (estadoManualVentilador) {
        digitalWrite(RELAY_BLUE, LOW);
        digitalWrite(LED_BLUE_PIN, HIGH);
        Serial.println("🚫 Controle automático BLOQUEADO — ventilador LIGADO manualmente.");
      } else {
        digitalWrite(RELAY_BLUE, HIGH);
        digitalWrite(LED_BLUE_PIN, LOW);
        Serial.println("🚫 Controle automático BLOQUEADO — ventilador DESLIGADO.");
      }
    }

    // Debug
    Serial.println("==== Leituras dos Sensores ====");
    Serial.printf("Gás (MQ2): %d [%s]\n", gas, (gas > MQ2_THRESHOLD) ? "ALERTA" : "OK");
    Serial.print("Relé Vermelho: "); Serial.println(digitalRead(RELAY_RED) == LOW ? "ATIVO" : "INATIVO");
    Serial.print("LED Vermelho: "); Serial.println(digitalRead(LED_RED_PIN) == HIGH ? "ACESO" : "APAGADO");
    Serial.print("Buzzer: "); Serial.println(digitalRead(BUZZER_PIN) == HIGH ? "ATIVO" : "INATIVO");
    Serial.println("==============================");
    Serial.print("Relé Azul: "); Serial.println(digitalRead(RELAY_BLUE) == LOW ? "ATIVO" : "INATIVO");
    Serial.print("LED Azul: "); Serial.println(digitalRead(LED_BLUE_PIN) == HIGH ? "ACESO" : "APAGADO");
    Serial.printf("Temperatura: %.2f °C (Threshold: %.2f °C)\n", temp, TEMP_THRESHOLD);
    Serial.println("==============================");
    Serial.print("Relé Branco: "); Serial.println(digitalRead(RELAY_WHITE) == LOW ? "ATIVO" : "INATIVO");
    Serial.print("Luzes: "); Serial.println(digitalRead(LED_WHITE_PIN) == HIGH ? "ACESAS" : "APAGADAS");
    Serial.printf("Luz (LDR): %d [%s]\n", ldr, (ldr < LDR_THRESHOLD) ? "Ambiente Escuro" : "Ambiente Claro");
    Serial.printf("Presença (PIR): %s\n", pres ? "DETECTADA" : "NÃO detectada");
    Serial.println("==============================");
  }

  delay(100);
}
