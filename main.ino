#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "SEU_SSID";
const char* password = "SUA_PASSWORD";
WebServer server(80);

const int ledVermelho = 4;
const int ledAmarelo = 5;
bool estadoVermelho = false;
int brilhoAmarelo = 0;

String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Controle de LEDs</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { 
      font-family: Arial; 
      text-align: center; 
      padding: 20px; 
      background: linear-gradient(135deg, #1e3c72, #2a5298);
      color: white;
    }
    h1 { 
      color: #FFD700; 
      text-shadow: 1px 1px 3px rgba(0,0,0,0.5);
    }
    h2 {
      color: #FFA500;
    }
    .card {
      background: rgba(255,255,255,0.1);
      border-radius: 10px;
      padding: 20px;
      margin: 20px auto;
      max-width: 400px;
      box-shadow: 0 4px 8px rgba(0,0,0,0.2);
      backdrop-filter: blur(5px);
    }
    button {
      padding: 12px 24px;
      margin: 10px;
      font-size: 18px;
      border: none;
      border-radius: 50px;
      cursor: pointer;
      transition: all 0.3s;
      color: white;
      box-shadow: 0 4px 8px rgba(0,0,0,0.2);
    }
    #btnOn {
      background: #FF5252;
    }
    #btnOn:hover {
      background: #FF0000;
      transform: translateY(-2px);
    }
    #btnOff {
      background: #455A64;
    }
    #btnOff:hover {
      background: #263238;
      transform: translateY(-2px);
    }
    #status {
      font-size: 20px;
      margin-top: 15px;
      padding: 10px;
      border-radius: 5px;
      background: rgba(0,0,0,0.2);
      display: inline-block;
    }
    input[type=range] {
      width: 80%;
      margin: 20px 0;
      -webkit-appearance: none;
      height: 15px;
      background: linear-gradient(to right, #FFA000, #FFD54F);
      border-radius: 10px;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 25px;
      height: 25px;
      background: #FF6F00;
      border-radius: 50%;
      cursor: pointer;
    }
    #valorPWM {
      font-weight: bold;
      color: #FFD54F;
      font-size: 24px;
    }
  </style>
</head>
<body>
  <h1>ESP32 LED Control</h1>
  
  <div class="card">
    <h2>LED Vermelho</h2>
    <button id="btnOn" onclick="enviarComando('/on')">LIGAR</button>
    <button id="btnOff" onclick="enviarComando('/off')">DESLIGAR</button>
    <div id="status">Status: --</div>
  </div>
  
  <div class="card">
    <h2>LED Amarelo (PWM)</h2>
    <input type="range" min="0" max="255" value="0" id="slider" oninput="mudarBrilho(this.value)">
    <div>Brilho: <span id="valorPWM">0</span></div>
  </div>
  
  <script>
    function enviarComando(caminho) {
      fetch(caminho)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerText = "Status: " + data;
          if(caminho === '/on') {
            document.getElementById('status').style.background = 'rgba(255,0,0,0.3)';
          } else {
            document.getElementById('status').style.background = 'rgba(0,0,0,0.2)';
          }
        });
    }
    
    function mudarBrilho(valor) {
      document.getElementById("valorPWM").innerText = valor;
      fetch('/pwm?val=' + valor);
      // Mudar a cor de fundo baseado no valor
      const brilhoPercent = valor / 255;
      document.getElementById("valorPWM").style.color = `rgb(255, ${Math.round(213 * brilhoPercent + 30)}, ${Math.round(79 * brilhoPercent)})`;
    }
    
    window.onload = () => {
      fetch('/estado')
        .then(r => r.text())
        .then(data => {
          document.getElementById('status').innerText = "Status: " + data;
          if(data.includes("LIGADO")) {
            document.getElementById('status').style.background = 'rgba(255,0,0,0.3)';
          }
        });
        
      fetch('/pwm-valor')
        .then(r => r.text())
        .then(data => {
          const valor = parseInt(data) || 0;
          document.getElementById("slider").value = valor;
          document.getElementById("valorPWM").innerText = valor;
        });
    }
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  pinMode(ledVermelho, OUTPUT);
  ledcAttachPin(ledAmarelo, 0);
  ledcSetup(0, 5000, 8);
  
  WiFi.begin(ssid, password);
  Serial.println("Conectando ao WiFi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Rotas
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });
  
  server.on("/on", HTTP_GET, []() {
    digitalWrite(ledVermelho, HIGH);
    estadoVermelho = true;
    server.send(200, "text/plain", "LED VERMELHO LIGADO");
  });
  
  server.on("/off", HTTP_GET, []() {
    digitalWrite(ledVermelho, LOW);
    estadoVermelho = false;
    server.send(200, "text/plain", "LED VERMELHO DESLIGADO");
  });
  
  server.on("/estado", HTTP_GET, []() {
    String estado = estadoVermelho ? "LED VERMELHO LIGADO" : "LED VERMELHO DESLIGADO";
    server.send(200, "text/plain", estado);
  });
  
  server.on("/pwm", HTTP_GET, []() {
    if (server.hasArg("val")) {
      brilhoAmarelo = server.arg("val").toInt();
      ledcWrite(0, brilhoAmarelo);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Valor ausente");
    }
  });
  
  server.on("/pwm-valor", HTTP_GET, []() {
    server.send(200, "text/plain", String(brilhoAmarelo));
  });
  
  server.begin();
  Serial.println("Servidor HTTP iniciado");
  Serial.println("Acesse: http://" + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
}
