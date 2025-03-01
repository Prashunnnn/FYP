#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>

// Wi-Fi Credentials
const char* ssid = "dursikshyaback";
const char* password = "CLB2722CC5";

// Weather API
String API_KEY = "92325fb9f39650118b62df5c4363b7a5";
String CITY = "Kathmandu";
String API_URL = "http://api.openweathermap.org/data/2.5/weather?q=Kathmandu&appid=" + API_KEY + "&units=metric";

// Web Server
AsyncWebServer server(80);

// Pins
#define SOIL_SENSOR_PIN 34
#define RELAY_PIN 23
#define BUTTON_PIN 18  // Optional manual button

bool onlineMode = false;
bool pumpState = false;

// Function to Fetch Weather Data
String getWeatherData() {
    HTTPClient http;
    http.begin(API_URL);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        String payload = http.getString();
        http.end();
        return payload;
    } else {
        http.end();
        return "Error Fetching Weather Data";
    }
}

// Function to Check Soil Moisture and Auto Control Pump
void checkSoilMoisture() {
    int moisture = analogRead(SOIL_SENSOR_PIN);
    moisture = constrain(moisture, 0, 3000);
    int moisturePercent = map(moisture, 3000, 0, 0, 100);
    Serial.print("Soil Moisture: ");
    Serial.print(moisturePercent);
    Serial.println("%");
    
    if (moisturePercent < 40) {
        digitalWrite(RELAY_PIN, LOW);  // Active-low relay (turns ON)
        pumpState = true;
    } else if (moisturePercent >= 40) {
        digitalWrite(RELAY_PIN, HIGH); // Active-low relay (turns OFF)
        pumpState = false;
    }
 else {
        digitalWrite(RELAY_PIN, HIGH); // Active-low relay (turns OFF)
        pumpState = false;
    }} 

// Function to Handle Web Requests
void startServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Irrigation</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f5f5f5;
            color: #333;
        }
        .container {
            margin: 20px auto;
            max-width: 400px;
            background: #fff;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.1);
        }
        h2 {
            font-size: 20px;
            margin-bottom: 10px;
        }
        .card {
            background: #ffeb3b;
            padding: 15px;
            margin: 10px 0;
            border-radius: 5px;
        }
        .button {
            display: inline-block;
            padding: 10px 20px;
            background: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
        }
        .button:hover {
            background: #45a049;
        }
        .pump-status {
            font-size: 18px;
            font-weight: bold;
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Smart Irrigation System</h1>
        <div class="card">
            <h2>Soil Moisture: <span id="moisture">--</span></h2>
        </div>
        <div class="card">
            <h2>Weather: <span id="weather">--</span></h2>
        </div>
        <div class="pump-status">Pump Status: <span id="pumpState">OFF</span></div>
        <button class="button" onclick="togglePump()">Toggle Pump</button>
    </div>

    <script>
        function togglePump() {
            fetch('/togglePump').then(response => response.text()).then(data => {
                document.getElementById("pumpState").innerText = data.includes("ON") ? "ON" : "OFF";
            });
        }
        
        function updateData() {
            fetch('/moisture').then(response => response.text()).then(data => {
                document.getElementById("moisture").innerText = data;
            });

            fetch('/weather').then(response => response.text()).then(data => {
                document.getElementById("weather").innerText = data;
            });

            fetch('/pumpStatus').then(response => response.text()).then(data => {
                document.getElementById("pumpState").innerText = data.includes("ON") ? "ON" : "OFF";
            });
        }

        setInterval(updateData, 2000);
    </script>
</body>
</html>
)rawliteral";

        request->send(200, "text/html", html);
    });

    server.on("/moisture", HTTP_GET, [](AsyncWebServerRequest *request) {
    int moisture = analogRead(SOIL_SENSOR_PIN);
    moisture = constrain(moisture, 0, 4095);
    int moisturePercent = map(moisture, 4095, 0, 0, 100);
    request->send(200, "text/plain", String(moisturePercent) + "%");
});

    server.on("/weather", HTTP_GET, [](AsyncWebServerRequest *request) {
    String weatherData = getWeatherData();
    if (weatherData.startsWith("Error")) {
        request->send(500, "text/plain", "Weather data not available");
        return;
    }
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, weatherData);
    if (error) {
        request->send(500, "text/plain", "JSON Parsing Error");
        return;
    }
    String temperature = String(doc["main"]["temp"]);
    String humidity = String(doc["main"]["humidity"]);
    String condition = String(doc["weather"][0]["description"]);
    String response = "Temperature: " + temperature + "°C "+ "\n";
    response += "Humidity: " + humidity + "% " + "\n";
    response += "Condition: " + condition + "";
    request->send(200, "text/plain", response);
});

    server.on("/pumpStatus", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", pumpState ? "ON" : "OFF");
    });

    server.on("/togglePump", HTTP_GET, [](AsyncWebServerRequest *request) {
        pumpState = !pumpState;
        if (pumpState) {
            digitalWrite(RELAY_PIN, LOW); // Active-low relay ON
        } else {
            digitalWrite(RELAY_PIN, HIGH); // Active-low relay OFF
        }
        request->send(200, "text/plain", pumpState ? "Pump ON" : "Pump OFF");
    });

    server.begin();
}

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SOIL_SENSOR_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(RELAY_PIN, HIGH);

    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to Wi-Fi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        onlineMode = true;
    } else {
        Serial.println("\nWi-Fi Not Available, Switching to Offline Mode");
        onlineMode = false;
        WiFi.softAP("ESP32_Offline", "12345678");
    }

    startServer();
}

void loop() {
    checkSoilMoisture();

    if (digitalRead(BUTTON_PIN) == LOW) {
        pumpState = !pumpState;
        digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW);
        delay(500);
    }

    delay(5000);
}
