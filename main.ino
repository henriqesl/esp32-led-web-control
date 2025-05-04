#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Evandro-UniaoFibra";
const char* password = "kenkaikakay";

WebServer server(80);

String statusLed = "DESLIGADO";
int pwmValor = 0;

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
      <head><meta charset="UTF-8"><title>Controle de LEDs</title></head>
      <body>
        <h1>Controle de LEDs</h1>
        <h2>LED Vermelho</h2>
        <p><a href="/on">LIGAR</a> | <a href="/off">DESLIGAR</a></p>
        <p>Status: %STATUS%</p>
        <h2>LED Amarelo (PWM)</h2>
        <form action="/pwm" method="GET">
          <label for="val">Brilho (0-255):</label>
          <input type="number" id="val" name="val" min="0" max="255" value="%PWM%">
          <input type="submit" value="Enviar">
        </form>
      </body>
    </html>
  )rawliteral";

  html.replace("%STATUS%", statusLed);
  html.replace("%PWM%", String(pwmValor));

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT);  // LED vermelho
  ledcAttachPin(5, 0); // LED amarelo PWM no pino 5 (canal 0)
  ledcSetup(0, 5000, 8); // 5kHz, resolução de 8 bits

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);

  server.on("/on", []() {
    digitalWrite(4, HIGH);
    statusLed = "LIGADO";
    handleRoot();
  });

  server.on("/off", []() {
    digitalWrite(4, LOW);
    statusLed = "DESLIGADO";
    handleRoot();
  });

  server.on("/pwm", []() {
    if (server.hasArg("val")) {
      pwmValor = server.arg("val").toInt();
      ledcWrite(0, pwmValor);
    }
    handleRoot();
  });

  server.begin();
}

void loop() {
  server.handleClient();
}
