# 🏠 ESP32 Smart Home Dashboard

Este projeto consiste em uma interface web (dashboard) responsiva para monitoramento e controle de dispositivos de automação residencial baseados no microcontrolador **ESP32**. A comunicação entre o hardware e o dashboard é realizada de forma assíncrona e em tempo real através do protocolo **MQTT**.

## 🚀 Funcionalidades

  * **Monitoramento em Tempo Real**: Gráficos dinâmicos para visualização de Temperatura, Presença de Gás e Luminosidade.
  * **Controle Remoto**: Botões para acionamento manual de iluminação (LED) e ventilação.
  * **Configuração de Limiares**: Permite definir via software os valores críticos para automação (ex: temperatura máxima para ligar o ventilador).
  * **Sistema de Alertas**: Notificações visuais instantâneas quando sensores detectam vazamentos ou ultrapassam limites definidos.
  * **Bloqueio de Sensores**: Funções específicas para desativar a lógica automática dos sensores de luz e temperatura diretamente pelo painel.

## 🛠️ Tecnologias Utilizadas

  * **Frontend**: HTML5, CSS3 (Modern UI com Gradientes e Cards).
  * **Comunicação**: [MQTT.js](https://www.google.com/search?q=https://github.com/mqttjs/MQTT.js) via WebSockets (WSS).
  * **Gráficos**: [Chart.js](https://www.chartjs.org/) para renderização de dados históricos.
  * **Broker MQTT**: `test.mosquitto.org` (Porta 8081).

## 📡 Tópicos MQTT Utilizados

O dashboard interage com os seguintes tópicos:

| Tipo | Tópico | Mensagem / Formato |
| :--- | :--- | :--- |
| **Sensor** | `casa/sensor/temperatura` | Valor numérico (ex: `25.5`) |
| **Sensor** | `casa/sensor/gas` | `ALERT` ou `SAFE` |
| **Sensor** | `casa/sensor/luz` | `DARK` ou `LIGHT` |
| **Comando** | `casa/command/led_branco` | `ON` ou `OFF` |
| **Comando** | `casa/command/ventilador` | `ON` ou `OFF` |
| **Config** | `casa/command/threshold/temp` | Valor numérico do limiar |
| **Config** | `casa/command/threshold/ldr` | Valor numérico do limiar |

## 💻 Como Rodar o Projeto

1.  Certifique-se de que o seu **ESP32** está configurado para conectar ao broker `test.mosquitto.org` na porta padrão (ou ajuste o código do dashboard se necessário).
2.  Faça o download ou clone este repositório.
3.  Abra o arquivo `index.html` em qualquer navegador moderno.
4.  O dashboard se conectará automaticamente via WebSockets.

> **Nota:** Como o projeto utiliza `wss://` (Websocket Seguro), certifique-se de que o navegador não bloqueie o script de conexão caso esteja rodando o arquivo localmente.

## 📂 Estrutura de Pastas

```text
├── index.html          # Interface única contendo HTML, CSS e JS
└── README.md           # Documentação do projeto
```

-----
