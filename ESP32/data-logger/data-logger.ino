// ======= INTEGRAÇÃO: Bluetooth config + DHT + OLED + MQTT (Fiware) =======

#include "BluetoothSerial.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ------------------ BLUETOOTH ------------------
BluetoothSerial SerialBT;

// ------------------ MQTT / FIWARE (constantes imutáveis) ------------------
const char* MQTT_BROKER   = "IP EC2 AWS";
const int   MQTT_PORT     = 1883;

const char* TOPICO_CMD    = "/TEF/lamp3/cmd";
const char* TOPICO_TEMP   = "/TEF/lamp3/attrs/t";
const char* TOPICO_HUM    = "/TEF/lamp3/attrs/h";
const char* TOPICO_LUM    = "/TEF/lamp3/attrs/l";

const char* MQTT_ID       = "fiware_sensor_full";

// ------------------ SENSORES + OLED ------------------
#define DHTPIN     32
#define DHTTYPE    DHT11
#define POT_PIN    34

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET     -1
#define OLED_ADDR      0x3C

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ------------------ VARIÁVEIS SENSORES ------------------
float temperatura = NAN;
float umidade = NAN;
int luminosidade = 0;

unsigned long tDht  = 0;
unsigned long tLum  = 0;
unsigned long tSend = 0;
unsigned long tSwap = 0;

const unsigned long kDhtMs   = 2000;
const unsigned long kLumMs   = 200;
const unsigned long kSendMs  = 2000;
const unsigned long kSwapMs  = 3000;

uint8_t tela = 0;

// ------------------ WI-FI (mutável via BT) ------------------
// Valores iniciais (fallback). Serão substituídos ao receber via BT.
String wifi_ssid = "";
String wifi_pass = "";

// ------------------ WI-FI + MQTT ------------------
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// ======================= FUNÇÕES AUXILIARES =======================

String getValue(String data, char separator, int index) {
  int start = 0;
  int end = data.indexOf(separator);

  for (int i = 0; i < index; i++) {
    start = end + 1;
    end = data.indexOf(separator, start);
    if (end == -1) {
      end = data.length();
      break;
    }
  }
  // proteger caso start > length
  if (start < 0) start = 0;
  if (start > data.length()) return "";
  if (end < start) end = data.length();
  return data.substring(start, end);
}

void printLCDCentered(int y, const char* text, uint8_t size = 1) {
  display.setTextSize(size);
  int16_t x1, y1; 
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

// ---------------- MQTT callback ----------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("[MQTT] CMD recebido em ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(msg);

  // Tratar comandos futuros aqui...
}

// ======================= CONEXÃO WI-FI =======================
void connectWiFi() {
  Serial.print("\nConectando ao WiFi ");
  Serial.println(wifi_ssid);

  display.clearDisplay();
  printLCDCentered(25, "Conectando WiFi...", 1);
  display.display();

  WiFi.disconnect(true); // limpa conexões anteriores
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
    // após 15s retorna e permite outras tentativas no loop
    if (millis() - start > 15000) {
      Serial.println("\nTimeout WiFi");
      display.clearDisplay();
      printLCDCentered(25, "Falha WiFi", 1);
      display.display();
      return;
    }
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();
  printLCDCentered(15, "WiFi conectado!", 1);
  printLCDCentered(35, WiFi.localIP().toString().c_str(), 1);
  display.display();
  delay(1000);
}

// ======================= CONEXÃO MQTT =======================
void connectMQTT() {
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  unsigned long start = millis();
  while (!mqtt.connected()) {
    Serial.print("Conectando ao broker MQTT... ");
    if (mqtt.connect(MQTT_ID)) {
      Serial.println("OK!");
      mqtt.subscribe(TOPICO_CMD);
      display.clearDisplay();
      printLCDCentered(25, "MQTT conectado!", 1);
      display.display();
      delay(500);
      return;
    } else {
      Serial.print("Falhou. Estado: ");
      Serial.println(mqtt.state());
      Serial.println("Tentando novamente em 2s...");
      delay(2000);
      // não travar o dispositivo: voltar e tentar novamente no loop principal
      if (millis() - start > 30000) {
        Serial.println("Timeout MQTT (retornando para loop).");
        return;
      }
    }
  }
}

// ======================= ENVIO PARA FIWARE =======================
void sendToFiware() {
  if (!mqtt.connected()) return;

  char tempStr[10];
  if (!isnan(temperatura)) snprintf(tempStr, sizeof(tempStr), "%.1f", temperatura);
  else strcpy(tempStr, "0.0");

  char humStr[10];
  if (!isnan(umidade)) snprintf(humStr, sizeof(humStr), "%.1f", umidade);
  else strcpy(humStr, "0.0");

  char lumStr[10];
  snprintf(lumStr, sizeof(lumStr), "%d", luminosidade);

  mqtt.publish(TOPICO_TEMP, tempStr);
  mqtt.publish(TOPICO_HUM, humStr);
  mqtt.publish(TOPICO_LUM, lumStr);

  Serial.println("---- DADOS ENVIADOS ----");
  Serial.print("T: "); Serial.println(tempStr);
  Serial.print("H: "); Serial.println(humStr);
  Serial.print("L: "); Serial.println(lumStr);
}

// ======================= AÇÃO AO RECEBER DADOS POR BLUETOOTH =======================
void handleBluetooth() {
  if (!SerialBT.available()) return;

  // lê tudo que veio (pode chegar em partes; readString pega buffer atual)
  String data = SerialBT.readString();
  data.trim(); // remove \r, \n e espaços nas pontas
  Serial.print("Recebido BT: ");
  Serial.println(data);

  // espera formato: ssid=MEU_WIFI;pass=MINHA_SENHA
  String part1 = getValue(data, ';', 0);
  String part2 = getValue(data, ';', 1);

  if (part1.startsWith("ssid=")) wifi_ssid = part1.substring(5);
  if (part2.startsWith("pass=")) wifi_pass = part2.substring(5);

  // limpa espaços indesejados
  wifi_ssid.trim();
  wifi_pass.trim();

  Serial.print("Novo SSID: ");
  Serial.println(wifi_ssid);
  Serial.print("Nova PASS: ");
  Serial.println(wifi_pass);

  // mostra no OLED
  display.clearDisplay();
  printLCDCentered(5, "Config Bluetooth", 1);
  display.setCursor(0, 18);
  display.setTextSize(1);
  display.print("SSID: ");
  display.println(wifi_ssid);
  display.print("PASS: ");
  display.println(wifi_pass);
  display.display();

  // tenta conectar imediatamente
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    connectMQTT();
  }
}

// ======================= OLED – DESENHO (mesmas funções do seu código) =======================
void drawHeader(const char* titulo) {
  display.setTextSize(1);
  int16_t x1, y1; 
  uint16_t w, h;
  display.getTextBounds(titulo, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 0);
  display.print(titulo);
}

void drawCenterBig(const char* valor, const char* unidade) {
  display.setTextSize(3);
  int16_t x1, y1; 
  uint16_t w, h;
  display.getTextBounds(valor, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, 20);
  display.print(valor);

  display.setTextSize(1);
  display.setCursor(x + w + 5, 28);
  display.print(unidade);
}

void drawTemp() {
  drawHeader("Temperatura");
  char buf[8];
  if (isnan(temperatura)) snprintf(buf, 8, "--");
  else snprintf(buf, 8, "%.1f", temperatura);
  drawCenterBig(buf, "C");
}

void drawHum() {
  drawHeader("Umidade");
  char buf[8];
  if (isnan(umidade)) snprintf(buf, 8, "--");
  else snprintf(buf, 8, "%.1f", umidade);
  drawCenterBig(buf, "%");
}

void drawLum() {
  drawHeader("Luminosidade");
  char buf[8];
  snprintf(buf, 8, "%d", luminosidade);
  drawCenterBig(buf, "%");
}

// ======================= SETUP =======================
void setup() {
  Serial.begin(115200);
  delay(100);

  // BLUETOOTH
  if (!SerialBT.begin("ESP32_Config")) {
    Serial.println("Erro ao iniciar Bluetooth");
    // Se preferir travar aqui, descomente:
    // while(true);
  } else {
    Serial.println("Bluetooth iniciado! Aguardando app...");
  }

  // SENSORES / DISPLAY
  dht.begin();
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("Falha ao inicializar OLED SSD1306"));
  }
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 25);
  display.print("Iniciando...");
  display.display();

  // Tenta conectar com as credenciais atuais (fallback)
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    connectMQTT();
  } else {
    Serial.println("Sem WiFi no setup - aguardar credenciais via BT");
    display.clearDisplay();
    printLCDCentered(25, "Aguardando WiFi via BT", 1);
    display.display();
  }
}

// ======================= LOOP PRINCIPAL =======================
void loop() {
  // trata Bluetooth (receber SSID/PASS)
  handleBluetooth();

  // mantém conexões MQTT/WiFi
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) {
      connectMQTT();
    } else {
      mqtt.loop();
    }
  } else {
    // tenta reconectar WiFi periodicamente (não travar)
    static unsigned long lastTryWiFi = 0;
    if (millis() - lastTryWiFi > 10000) { // 10s
      lastTryWiFi = millis();
      connectWiFi();
    }
  }

  unsigned long now = millis();

  // --- DHT (2s) ---
  if (now - tDht >= kDhtMs) {
    tDht = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      umidade = h;
      temperatura = t;
    } else {
      Serial.println("Falha leitura DHT!");
    }
  }

  // --- Luminosidade (200ms) ---
  if (now - tLum >= kLumMs) {
    tLum = now;
    int raw = analogRead(POT_PIN);
    luminosidade = map(raw, 0, 4095, 0, 100);
  }

  // --- Enviar FIWARE (2s) ---
  if (now - tSend >= kSendMs) {
    tSend = now;
    sendToFiware();
  }

  // --- Trocar telas (3s) ---
  if (now - tSwap >= kSwapMs) {
    tSwap = now;
    tela = (tela + 1) % 3;
  }

  // --- Render OLED ---
  display.clearDisplay();
  if (tela == 0)      drawTemp();
  else if (tela == 1) drawHum();
  else                drawLum();
  display.display();

  // pequeno sleep para evitar loop apertado
  delay(10);
}

------------------------------------------------------------------------