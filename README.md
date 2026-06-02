# Casa Inteligente com ESP32 — README

---

## Descrição

Sistema de automação residencial baseado em **ESP32** que monitora temperatura, luminosidade e presença, controlando automaticamente um ventilador e uma luz via relés. Os dados são publicados em tempo real via **MQTT (HiveMQ Cloud)** e o dispositivo também expõe uma **API HTTP local**.

---

## Hardware necessário

| Componente | Pino ESP32 |
|---|---|
| Sensor DHT11 (temperatura/umidade) | GPIO 15 |
| Sensor LDR (luminosidade) | GPIO 35 |
| Sensor PIR (presença) | GPIO 14 |
| Relé 1 — Ventilador | GPIO 16 |
| Relé 2 — Luz | GPIO 17 |
| LED indicador | GPIO 26 |

---

## Dependências (Arduino IDE / PlatformIO)

```
WiFi.h
WiFiClientSecure.h
PubSubClient.h
DHT.h
WebServer.h
```

---

## Lógica de automação

**Luz (Relé 2):** Liga automaticamente quando o ambiente está escuro (`LDR < threshold`) **e** há presença detectada pelo PIR.

**Ventilador (Relé 1):** Liga automaticamente quando a temperatura ultrapassa `TEMP_THRESHOLD` (padrão: 28°C).

Ambos os comportamentos podem ser **bloqueados via MQTT** para controle manual.

---

## Tópicos MQTT

### Publicação (ESP32 → Broker)

| Tópico | Valor | Descrição |
|---|---|---|
| `casa/sensor/temperatura` | `25.3` | Temperatura em °C |
| `casa/sensor/luz` | `DARK` / `BRIGHT` | Estado da luminosidade |
| `casa/sensor/presenca` | `1` / `0` | Presença detectada |

### Comandos (Broker → ESP32)

| Tópico | Valor | Descrição |
|---|---|---|
| `casa/command/ventilador` | `ON` / `OFF` | Controle manual do ventilador |
| `casa/command/led_branco` | `ON` / `OFF` | Controle manual da luz |
| `casa/command/threshold/temp` | ex: `30.0` | Altera o limite de temperatura |
| `casa/command/threshold/ldr` | ex: `2000` | Altera o limite do LDR |
| `casa/command/bloqueio_sensor_luz` | `ON` / `OFF` | Desativa automação da luz |
| `casa/command/bloqueio_sensor_temp` | `ON` / `OFF` | Desativa automação do ventilador |

> Os dados são publicados a cada **10 segundos**.

---

## API HTTP local

Com o ESP32 conectado na rede, acesse pelo IP local:

| Endpoint | Método | Descrição |
|---|---|---|
| `/toggle-luz` | GET | Alterna o estado da luz |
| `/set-threshold?value=30.0` | GET | Define novo limite de temperatura |

---

## Configuração

No início do arquivo `.ino`, altere as credenciais conforme seu ambiente:

```cpp
// WiFi
const char* WIFI_SSID = "SUA_REDE";
const char* WIFI_PASS = "SUA_SENHA";

// MQTT
const char* MQTT_BROKER = "seu.broker.hivemq.cloud";
const char* MQTT_USER   = "usuario";
const char* MQTT_PASS   = "senha";
```

---

## Fluxo geral

```
ESP32 inicializa
  ├── Conecta ao WiFi
  ├── Sobe servidor HTTP
  └── Conecta ao broker MQTT
        └── A cada 10s:
              ├── Lê DHT11, LDR, PIR
              ├── Publica dados no broker
              ├── Aciona luz (se automação ativa)
              └── Aciona ventilador (se automação ativa)
```

---

## Observações

- A conexão MQTT usa TLS com `setInsecure()` (sem validação de certificado). Para produção, utilize um certificado CA válido.
- O LED no GPIO 26 é compartilhado entre os dois relés — considere usar pinos separados se precisar de indicação individual.
