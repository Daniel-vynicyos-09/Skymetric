#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_BMP085.h>
#include "DHT.h"

// ===== CONFIGURE AQUI =====
#define WIFI_SSID     "SEU_WIFI"
#define WIFI_PASS     "SENHA_WIFI"

#define NOME_ESTACAO  "SkymetricV3"
#define URL_SERVIDOR  "http://SEU_SERVIDOR/estacao"  // exemplo: http://192.168.0.10:8080/estacao

// ===== SENSORES =====
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

Adafruit_BMP085 bmp;   // BMP180

// ===== UV (analógico) =====
#define UV_PIN 34  

// Converte UV analógico para índice UV aproximado
int converterUV(int valorADC) {

  float tensao = (valorADC / 4095.0) * 3.3;

  if (tensao < 0.99) return 0;
  if (tensao < 1.5) return 1;
  if (tensao < 2.0) return 2;
  if (tensao < 2.5) return 3;
  if (tensao < 2.8) return 4;
  if (tensao < 3.1) return 5;

  return 6; // máximo
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  if (!bmp.begin()) {
    Serial.println("ERRO: BMP NAO ENCONTRADO FERIFIQUE AS LIGACOES!");
    while (1);
  }

  // Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

void loop() {

  // ======== LEITURA DOS SENSORES ========
  float umidade = dht.readHumidity();                  // DHT11 → Umidade
  float temp_ar = bmp.readTemperature();                // BMP180 → Temperatura
  int pressao = bmp.readPressure() / 100;               // BMP180 → Pressão hPa

  int uvADC = analogRead(UV_PIN);                       // UV sensor
  int indiceUV = converterUV(uvADC);

  // Tratamento de erros
  if (isnan(umidade)) {
    Serial.println("Erro ao ler DHT11!");
    umidade = 0;
  }

  Serial.println("===== Dados Lidos =====");
  Serial.printf("Umidade: %.0f %%\n", umidade);
  Serial.printf("Temperatura: %.2f °C\n", temp_ar);
  Serial.printf("Pressão: %d hPa\n", pressao);
  Serial.printf("UV Index: %d\n\n", indiceUV);

  // ======== MONTA JSON ========
  String json = "{";
  json += "\"nome\":\"" + String(NOME_ESTACAO) + "\",";
  json += "\"temperaturaAr\":" + String(temp_ar, 2) + ",";
  json += "\"umidadeAr\":" + String((int)umidade) + ",";
  json += "\"pressaoAr\":" + String(pressao) + ",";
  json += "\"indice_uv\":" + String(indiceUV);
  json += "}";

  // ======== ENVIA PARA SERVIDOR ========
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(URL_SERVIDOR);
    http.addHeader("Content-Type", "application/json");

    int codigo = http.POST(json);

    Serial.print("Resposta servidor: ");
    Serial.println(codigo);

    http.end();
  }

  delay(6000); // envia a cada 5 segundos 
}
