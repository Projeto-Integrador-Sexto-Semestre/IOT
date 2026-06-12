# Smart Home IoT - ESP32

## Informações do Projeto

**Nome do Projeto:** SMARTHOME

**Integrantes do Projeto:**
- Victor Daniel Araújo Silva
- Gustavo Henrique Santana dos Santos
- Camille Alves Cruz
- Lucas Garcia Lima
- Raphael Micucci Bomfim
- Isaías Belarmina de Souza

**Tema Escolhido:** Sistema de Monitoramento de Casa Inteligente (Smart Home)

## Descrição do Problema Resolvido

Atualmente, muitas residências possuem dispositivos inteligentes e sensores capazes de coletar informações sobre o ambiente, porém essas informações nem sempre ficam centralizadas em uma única plataforma, dificultando o monitoramento e o gerenciamento dos dispositivos. Este projeto propõe uma plataforma unificada para monitorar dispositivos e sensores, permitindo o acompanhamento em tempo real, o gerenciamento dos equipamentos e o recebimento de alertas quando eventos importantes forem detectados.

## Divisão de Responsabilidades

| Integrante | Responsabilidade |
|---|---|
| Victor Daniel Araújo Silva | Backend API |
| Camille Alves Cruz | Backend API e Testes Unitários |
| Lucas Garcia Lima | IoT e JWT (API) |
| Isaías Belarmina de Souza | Mobile e Web |
| Gustavo Henrique Santana dos Santos | Testes |
| Raphael Micucci Bomfim | Documentação |

---

## Visão Geral

Firmware Arduino para o módulo IoT do projeto **PI Smart Home**. O sketch roda em um ESP32, coleta leituras de temperatura, luminosidade e presença, aciona relés para ventilador e luz, publica telemetria via MQTT e expõe endpoints HTTP locais para operações diretas.

Este repositório representa a camada física do sistema de casa inteligente. O ESP32 atua como gateway dos sensores e atuadores:

- lê temperatura pelo sensor DHT11;
- lê luminosidade por LDR;
- detecta presença por PIR;
- aciona um relé para ventilador;
- aciona um relé para iluminação;
- publica leituras no broker MQTT;
- recebe comandos remotos pelo MQTT;
- permite controle básico pela rede local via HTTP.

## Hardware

| Componente | Pino ESP32 | Uso |
|---|---:|---|
| DHT11 | GPIO 15 | Temperatura |
| LDR | GPIO 35 | Luminosidade analógica |
| PIR | GPIO 14 | Presença/movimento |
| Relé 1 | GPIO 16 | Ventilador |
| Relé 2 | GPIO 17 | Luz |
| LED indicador | GPIO 26 | Indicação visual dos acionamentos |

> Observação: no código atual, `LED_BLUE_PIN` e `LED_WHITE_PIN` usam o mesmo GPIO 26. Isso funciona como indicador compartilhado, mas não permite sinalização independente dos dois relés.

## Dependências

Instale as bibliotecas abaixo pela Arduino IDE ou pelo PlatformIO:

- `WiFi`
- `WiFiClientSecure`
- `PubSubClient`
- `DHT sensor library`
- `WebServer`

Placa esperada: ESP32.

## Configuração

Antes de gravar o firmware, ajuste as credenciais no início do arquivo `main.ino`:

```cpp
const char* WIFI_SSID = "SUA_REDE_WIFI";
const char* WIFI_PASS = "SENHA_DA_SUA_REDE_WIFI";

const char* MQTT_BROKER = "seu-broker.hivemq.cloud";
const uint16_t MQTT_PORT = 8883;
const char* MQTT_USER = "usuario";
const char* MQTT_PASS = "senha";
```

Também é possível ajustar os limites padrão:

```cpp
int LDR_THRESHOLD = 1500;
float TEMP_THRESHOLD = 28.0;
```

## Variáveis de Ambiente Necessárias

Configure as seguintes variáveis no arquivo `main.ino` antes de gravar o firmware:

| Variável | Descrição | Tipo |
|---|---|---|
| `WIFI_SSID` | Nome da rede Wi-Fi | String |
| `WIFI_PASS` | Senha da rede Wi-Fi | String |
| `MQTT_BROKER` | Endereço do broker MQTT | String |
| `MQTT_PORT` | Porta do broker MQTT | Inteiro |
| `MQTT_USER` | Usuário MQTT (opcional) | String |
| `MQTT_PASS` | Senha MQTT | String |
| `LDR_THRESHOLD` | Limite de luminosidade (padrão: 1500) | Inteiro |
| `TEMP_THRESHOLD` | Limite de temperatura (padrão: 28.0°C) | Float |

## Exemplos de Configuração para Teste

**Configuração Local (protótipo):**
```cpp
const char* WIFI_SSID = "SEU_SSID_AQUI";
const char* WIFI_PASS = "SUA_SENHA_AQUI";
const char* MQTT_BROKER = "broker.local";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "mqtt_user";
const char* MQTT_PASS = "mqtt_password";
```

**Limites de Sensores (padrão):**
- LDR_THRESHOLD = 1500 (luminosidade)
- TEMP_THRESHOLD = 28.0°C (temperatura)

## Como Executar

1. Abra `main.ino` na Arduino IDE.
2. Selecione a placa ESP32 correta.
3. Instale as bibliotecas necessárias.
4. Configure Wi-Fi e MQTT.
5. Grave o sketch na placa.
6. Abra o Monitor Serial em `115200`.

Ao iniciar, o ESP32 conecta no Wi-Fi, mostra o IP local no Serial, inicia o servidor HTTP na porta 80 e tenta conectar ao broker MQTT.

## Telemetria MQTT

O ESP32 publica as leituras a cada 10 segundos.

| Tópico | Payload | Descrição |
|---|---|---|
| `casa/sensor/temperatura` | Ex.: `25.3` | Temperatura em graus Celsius |
| `casa/sensor/luz` | `DARK` ou `BRIGHT` | Estado calculado da luminosidade |
| `casa/sensor/presenca` | `1` ou `0` | Presença detectada pelo PIR |

As mensagens são publicadas com flag `retained`, permitindo que novos clientes recebam a última leitura conhecida.

## Comandos MQTT

| Tópico | Payload | Efeito |
|---|---|---|
| `casa/command/ventilador` | `ON` ou `OFF` | Define o estado manual do ventilador quando a automação de temperatura está bloqueada |
| `casa/command/led_branco` | `ON` ou `OFF` | Liga ou desliga a luz |
| `casa/command/threshold/temp` | Ex.: `30.0` | Atualiza o limite de temperatura |
| `casa/command/threshold/ldr` | Ex.: `2000` | Atualiza o limite do LDR |
| `casa/command/bloqueio_sensor_luz` | `ON` ou `OFF` | Bloqueia ou libera automação da luz |
| `casa/command/bloqueio_sensor_temp` | `ON` ou `OFF` | Bloqueia ou libera automação do ventilador |

## Regras de Automação

### Luz

A luz liga automaticamente quando:

- a automação de luz não está bloqueada;
- o LDR indica ambiente escuro (`ldr < LDR_THRESHOLD`);
- o sensor PIR detecta presença.

Quando essas condições deixam de ser verdadeiras, a luz é desligada.

### Ventilador

O ventilador liga automaticamente quando:

- a automação de temperatura não está bloqueada;
- a leitura do DHT11 é válida;
- a temperatura lida é maior que `TEMP_THRESHOLD`.

Quando a automação de temperatura está bloqueada, o estado passa a seguir o comando manual recebido em `casa/command/ventilador`.

## API HTTP Local

Depois que o ESP32 estiver conectado, use o IP exibido no Monitor Serial.

| Método | Endpoint | Descrição |
|---|---|---|
| `GET` | `/toggle-luz` | Alterna o estado atual da luz |
| `GET` | `/set-threshold?value=30.0` | Atualiza o limite de temperatura |

Exemplo:

```bash
curl "http://IP_DO_ESP32/toggle-luz"
curl "http://IP_DO_ESP32/set-threshold?value=30.0"
```

## Fluxo de Funcionamento

```text
ESP32 inicia
  -> conecta ao Wi-Fi
  -> inicia servidor HTTP local
  -> conecta ao broker MQTT
  -> assina tópicos de comando
  -> a cada 10 segundos:
       -> lê DHT11, LDR e PIR
       -> publica telemetria
       -> aplica regras de automação
       -> registra status no Serial
```

## Integração com os Outros Módulos

- O **Backend** pode assinar os tópicos `casa/sensor/*`, persistir leituras e publicar comandos em `casa/command/*`.
- O **Web** e o **Mobile** consomem dados do backend e podem acionar comandos que chegam ao ESP32 via MQTT.

## Cuidados

- O código usa `wifiClient.setInsecure()`, ou seja, conecta via TLS sem validar certificado. Para produção, configure o certificado CA do broker.
- Evite versionar credenciais reais de Wi-Fi ou MQTT. Use placeholders no repositório e mantenha segredos fora do código público.
- Verifique o tipo do módulo de relé usado. O código considera relé ativo em nível baixo (`LOW` liga, `HIGH` desliga).
