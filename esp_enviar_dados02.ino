#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include "time.h"

// Credenciais WiFi
const char* ssid = "Vodafone-F0A9D7";
const char* password = "MFDXDG9T4XM96FET";

//const char* ssid     = "RedmiHB";
//const char* password = "HB_123456";

// Configurações do NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Atualiza a cada 60 segundos

// URL do script do Google Apps
const char* googleScriptUrl = "https://script.google.com/macros/s/AKfycbzis3DS-BXkml7QyPEqpYJZjPMd2d30sDd_NsEZ9jh8FN8TIQR8IiaVzzqkGPBmN4vFSw/exec";

void setupSerial() {
  Serial.begin(115200);       // Serial Monitor
  Serial1.begin(9600, SERIAL_8N1, 1, 3); // Comunicação com o Mega
  Serial.println("Serial inicializada");
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConectado!");
}

void sendDataToGoogleSheets(String name, String tagId, String dateTime) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(googleScriptUrl);

    // Formatar os dados
    String postData = "name=" + name + "&tagId=" + tagId + "&dateTime=" + dateTime;

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      Serial.println("Dados enviados com sucesso!");
      Serial.println("Resposta: " + http.getString());
    } else {
      Serial.println("Erro ao enviar dados: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("WiFi desconectado. Tentando reconectar...");
    setupWiFi();
  }
}

void setup() {
  setupSerial();
  setupWiFi();

  // Inicializa o cliente NTP
  timeClient.begin();
  Serial.println("Inicialização concluída");
}

void loop() {
  // Atualiza a hora atual
  timeClient.update();

  // Obtém o tempo formatado
  String formattedTime = timeClient.getFormattedTime();

  // Obtém o dia e o mês
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti = localtime(&rawtime);
  int day = ti->tm_mday;
  int month = ti->tm_mon + 1; // tm_mon retorna meses desde janeiro (0-11), então adicionamos 1

  // Cria uma string no formato "dd/mm hh:mm"
  char buffer[16];
  sprintf(buffer, "%02d/%02d %s", day, month, formattedTime.c_str());
  String dateTime = String(buffer);

  // Envia a data/hora para o Mega
  Serial1.println(dateTime);
  Serial.println("Data e hora enviadas para o Mega: " + dateTime);

  // Verifica se há dados recebidos do Mega
  if (Serial1.available()) {
    String receivedData = Serial1.readStringUntil('\n');
    receivedData.trim(); // Remove espaços extras

    // Divide os dados recebidos
    int separator = receivedData.indexOf(';');

    String name = receivedData.substring(0, separator);
    String tagId = receivedData.substring(separator + 1);

    // Valida os dados recebidos
    if (name.length() > 0 && tagId.length() > 0) {
      sendDataToGoogleSheets(name, tagId, dateTime);
    } else {
      Serial.println("Dados incompletos recebidos, ignorando.");
    }

    Serial.println("Dados recebidos do Mega: " + receivedData);
  }

  delay(200); // Pequeno atraso para leitura
}
