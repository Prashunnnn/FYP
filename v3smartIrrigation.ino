#include <DHT.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Wi-Fi Configuration
const char* ssid = "prashun";
const char* password = "12345678";
const char* serverName = "http://192.168.101.7/moisture_project/store_moisture.php";

// Weather API
const String API_KEY = "92325fb9f39650118b62df5c4363b7a5";
String CITY = "Kathmandu";
String API_URL = "http://api.openweathermap.org/data/2.5/weather?q=" + CITY + "&appid=" + API_KEY + "&units=metric";
String FORECAST_URL = "http://api.openweathermap.org/data/2.5/forecast?q=" + CITY + "&appid=" + API_KEY + "&units=metric";

// Telegram
const char* telegramBotToken = "8178345756:AAEvZv-38_BJGtn9SB4ZLRjv9Kj-m2jCXZs";
const char* telegramChatID = "5784424361";

// Hardware Pins
#define SOIL_SENSOR_PIN 34
#define RELAY_PIN 23
#define BUTTON_PIN 18
#define DHTPIN 22
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);

// Global Variables
const int threshold = 40;
unsigned long lastSendTime = 0;
const long sendInterval = 30000;
unsigned long lastToggleTime = 0;
const unsigned long debounceDelay = 1000;
unsigned long lastWeatherUpdate = 0;
const long weatherUpdateInterval = 600000;

bool onlineMode = false;
bool pumpState = false;
bool alertSent = false;
int moisturePercent = 0;
float temp = 0.0;
float humidity = 0.0;

String currentWeather = "{}";
String forecastData = "{}";

// Function Prototypes
void handleRelay(bool state);
void updateWeatherData();
void handleSensorData(AsyncWebServerRequest *request);
void startServer();
void sendMoistureData(int moisture, float temp, float humidity);
void sendAlert();
String urlEncode(String str);

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);
    pinMode(SOIL_SENSOR_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    dht.begin();

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    for(int i=0; i<15; i++) {
        if(WiFi.status() == WL_CONNECTED) break;
        delay(1000);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        onlineMode = true;
        Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
        updateWeatherData();
    } else {
        Serial.println("\nStarting AP Mode");
        WiFi.softAP("ESP32-Irrigation", "12345678");
    }

    startServer();
}

void loop() {
    unsigned long currentMillis = millis();
    
    // Update weather every 10 minutes
    if (currentMillis - lastWeatherUpdate >= weatherUpdateInterval) {
        updateWeatherData();
        lastWeatherUpdate = currentMillis;
    }

    // Moisture management
    static unsigned long lastMoistureCheck = 0;
    if (currentMillis - lastMoistureCheck >= 2000) {
        int moisture = analogRead(SOIL_SENSOR_PIN);
        moisturePercent = map(constrain(moisture, 2500, 4095), 4095, 2500, 0, 100);
        
        if (moisturePercent < threshold && !pumpState) {
            handleRelay(true);
        } else if (moisturePercent >= threshold && pumpState) {
            handleRelay(false);
        }
        
        lastMoistureCheck = currentMillis;
    }

    // Data sending
    if (onlineMode && (currentMillis - lastSendTime >= sendInterval)) {
        sendMoistureData(moisturePercent, temp, humidity);
        lastSendTime = currentMillis;
    }

    // Alert handling
    if (moisturePercent < threshold && !alertSent) {
        sendAlert();
        alertSent = true;
    } else if (moisturePercent >= threshold) {
        alertSent = false;
    }

    delay(100);
}

void handleRelay(bool state) {
    if (state) {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW); // Active LOW - turn ON
    } else {
        pinMode(RELAY_PIN, INPUT_PULLUP); // Disable relay
        digitalWrite(RELAY_PIN, HIGH);    // Optional - for clarity
    }
    pumpState = state;
    Serial.println("Pump " + String(state ? "ON" : "OFF"));
}

void updateWeatherData() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    
    // Update current weather
    http.begin(API_URL);
    if (http.GET() == HTTP_CODE_OK) {
        currentWeather = http.getString();
        Serial.println("Current weather updated");
    }
    http.end();

    // Update forecast
    http.begin(FORECAST_URL);
    if (http.GET() == HTTP_CODE_OK) {
        forecastData = http.getString();
        Serial.println("Forecast updated");
    }
    http.end();
}

void handleSensorData(AsyncWebServerRequest *request) {
    DynamicJsonDocument json(2048);
    
    int moisture = analogRead(SOIL_SENSOR_PIN);
    moisturePercent = map(constrain(moisture, 2500, 4095), 4095, 2500, 0, 100);
    temp = dht.readTemperature();
    humidity = dht.readHumidity();

    json["moisture"] = moisturePercent;
    json["temperature"] = isnan(temp) ? 0.0 : temp;
    json["humidity"] = isnan(humidity) ? 0.0 : humidity;
    json["pump"] = pumpState;

    DynamicJsonDocument weatherDoc(1024);
    deserializeJson(weatherDoc, currentWeather);
    json["pressure"] = weatherDoc["main"]["pressure"] | 0;
    json["wind_speed"] = weatherDoc["wind"]["speed"] | 0.0;
    json["condition"] = weatherDoc["weather"][0]["description"].as<String>();

    DynamicJsonDocument forecastDoc(4096);
    deserializeJson(forecastDoc, forecastData);
    JsonArray forecastArray = json.createNestedArray("forecast");
    
    if (forecastDoc.containsKey("list")) {
        for (int i = 0; i < 3; i++) {
            JsonObject item = forecastDoc["list"][i];
            JsonObject forecastItem = forecastArray.createNestedObject();
            forecastItem["time"] = item["dt_txt"].as<String>();
            forecastItem["temp"] = item["main"]["temp"];
            forecastItem["condition"] = item["weather"][0]["description"].as<String>();
        }
    }

    String response;
    serializeJson(json, response);
    request->send(200, "application/json", response);
}

void sendMoistureData(int moisture, float temp, float humidity) {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    WiFiClient client;
    
    String postData = "moisture=" + String(moisture) + 
                     "&temperature=" + String(temp) + 
                     "&humidity=" + String(humidity);

    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpCode = http.POST(postData);
    if (httpCode > 0) {
        Serial.printf("Data sent: %d\n", httpCode);
    } else {
        Serial.printf("Data send failed: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

void sendAlert() {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client;
    client.setInsecure();
    
    String message = "🚨 Alert! Soil Moisture Low: " + String(moisturePercent) + "%";
    String url = "https://api.telegram.org/bot" + String(telegramBotToken) +
                "/sendMessage?chat_id=" + String(telegramChatID) +
                "&text=" + urlEncode(message);

    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        Serial.println("Alert sent");
    } else {
        Serial.println("Alert failed: " + String(httpCode));
    }
    http.end();
}

String urlEncode(String str) {
    String encoded = "";
    char c;
    for (unsigned int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else if (c == ' ') {
            encoded += "%20";
        } else {
            encoded += "%" + String(String(c, HEX).c_str());
        }
    }
    return encoded;
}

void startServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Irrigation System</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <style>
    body {
        background: linear-gradient(135deg, #C7F0BD 0%, #9BC490 100%);
        color: #2c3e50;
        min-height: 100vh;
        padding: 20px;
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    }
    .card {
        background: rgba(255, 255, 255, 0.85);
        backdrop-filter: blur(10px);
        border-radius: 15px;
        padding: 20px;
        margin: 10px;
        box-shadow: 0 4px 30px rgba(0, 0, 0, 0.1);
        transition: all 0.3s ease; /* NEW */
        cursor: pointer; /* NEW */
    }
    .card:hover { /* NEW */
        transform: translateY(-5px);
        box-shadow: 0 8px 30px rgba(0, 0, 0, 0.2);
        background: rgba(255, 255, 255, 0.95);
    }
    .btn-toggle {
        background: #8BBF8A;
        border: none;
        padding: 12px 24px;
        font-size: 1.1em;
        transition: all 0.3s ease;
        color: white;
    }
    .btn-toggle:hover {
        background: #75A773;
        transform: scale(1.05);
        box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2); /* NEW */
    }
    #forecast-table {
        background: rgba(255, 255, 255, 0.8);
        border-radius: 10px;
        overflow: hidden;
    }
    #forecast-table tr { /* NEW */
        transition: background-color 0.3s ease;
    }
    #forecast-table tr:hover { /* NEW */
        background-color: rgba(255, 255, 255, 0.9);
    }
    .pump-box { /* NEW */
        transition: all 0.3s ease;
    }
    .pump-box:hover { /* NEW */
        transform: scale(1.02);
    }
</style>
</head>
<body>
    <h1 class="text-center mb-4">🌱 Smart Irrigation System</h1>

    <div class="container">
        <div class="city-input mb-4">
            <input type="text" id="city-input" placeholder="Enter City" class="form-control">
            <button onclick="updateCity()" class="btn btn-success mt-2">Update City</button>
        </div>

        <div class="row g-4">
            <div class="col-md-4">
                <div class="card">
                    <h3>🌱 Soil Moisture</h3>
                    <p class="display-4 fw-bold" id="moisture">--%</p>
                </div>
            </div>
            <div class="col-md-4">
                <div class="card">
                    <h3>🌡️ Temperature</h3>
                    <p class="display-4 fw-bold" id="temperature">--°C</p>
                </div>
            </div>
            <div class="col-md-4">
                <div class="card">
                    <h3>💧 Humidity</h3>
                    <p class="display-4 fw-bold" id="humidity">--%</p>
                </div>
            </div>
        </div>

        <div class="row mt-4">
            <div class="col-md-6">
                <div class="card">
                    <h3>🌦️ Current Weather</h3>
                    <p><strong>Pressure:</strong> <span id="pressure">--</span> hPa</p>
                    <p><strong>Condition:</strong> <span id="condition">--</span></p>
                    <p><strong>Wind Speed:</strong> <span id="wind_speed">--</span> km/h</p>
                </div>
            </div>
            <div class="col-md-6">
                <div class="card">
                    <h3>⚡ Pump Control</h3>
                    <div class="pump-box">
                        <p>Status: <span id="pump-status" class="fw-bold">OFF</span></p>
                        <button id="toggle-button" class="btn btn-toggle w-100">Toggle Pump</button>
                    </div>
                </div>
            </div>
        </div>

        <div class="card mt-4">
            <h3>📅 Weather Forecast</h3>
            <div class="table-responsive">
                <table class="table" id="forecast-table">
                    <thead>
                        <tr>
                            <th>Date/Time</th>
                            <th>Temp (°C)</th>
                            <th>Conditions</th>
                        </tr>
                    </thead>
                    <tbody>
                        <!-- Forecast data will be inserted here -->
                    </tbody>
                </table>
            </div>
        </div>
    </div>

    <script>
        function fetchData() {
            fetch('/sensor-data')
                .then(response => {
                    if (!response.ok) throw new Error('Network error');
                    return response.json();
                })
                .then(data => {
                    updateSensorData(data);
                    updateForecastTable(data.forecast);
                })
                .catch(error => {
                    console.error('Fetch error:', error);
                    document.getElementById('moisture').innerHTML = '⚠️';
                });
        }

        function updateSensorData(data) {
            document.getElementById('moisture').textContent = `${data.moisture}%`;
            document.getElementById('temperature').textContent = `${data.temperature.toFixed(1)}°C`;
            document.getElementById('humidity').textContent = `${data.humidity.toFixed(1)}%`;
            document.getElementById('pressure').textContent = data.pressure;
            document.getElementById('wind_speed').textContent = (data.wind_speed * 3.6).toFixed(1);
            document.getElementById('condition').textContent = data.condition;
            
            const pumpStatus = document.getElementById('pump-status');
            pumpStatus.textContent = data.pump ? 'ON' : 'OFF';
            pumpStatus.className = data.pump ? 'text-success' : 'text-danger';
        }

        function updateForecastTable(forecast) {
            const tbody = document.querySelector('#forecast-table tbody');
            tbody.innerHTML = '';
            
            if(forecast && forecast.length > 0) {
                forecast.forEach(item => {
                    const row = document.createElement('tr');
                    row.innerHTML = `
                        <td>${formatDateTime(item.time)}</td>
                        <td>${item.temp.toFixed(1)}°C</td>
                        <td>${getWeatherIcon(item.condition)} ${item.condition}</td>
                    `;
                    tbody.appendChild(row);
                });
            } else {
                tbody.innerHTML = `
                    <tr>
                        <td colspan="3" class="text-center">No forecast data available</td>
                    </tr>
                `;
            }
        }

        function formatDateTime(str) {
            const date = new Date(str);
            return date.toLocaleString('en-US', {
                weekday: 'short',
                month: 'short',
                day: 'numeric',
                hour: 'numeric',
                minute: '2-digit'
            });
        }

        function getWeatherIcon(condition) {
            condition = condition.toLowerCase();
            if (condition.includes('cloud')) return '☁️';
            if (condition.includes('rain')) return '🌧️';
            if (condition.includes('sun') || condition.includes('clear')) return '☀️';
            if (condition.includes('snow')) return '❄️';
            if (condition.includes('storm')) return '⛈️';
            return '🌫️';
        }

        function updateCity() {
            const city = document.getElementById('city-input').value;
            fetch(`/update-city?city=${encodeURIComponent(city)}`)
                .then(response => response.text())
                .then(() => fetchData())
                .catch(console.error);
        }

        document.getElementById('toggle-button').addEventListener('click', () => {
            fetch('/toggle-pump')
                .then(response => response.text())
                .then(status => {
                    const pumpStatus = document.getElementById('pump-status');
                    pumpStatus.textContent = status;
                    pumpStatus.className = status === 'ON' ? 'text-success' : 'text-danger';
                })
                .catch(console.error);
        });

        setInterval(fetchData, 2000);
        fetchData();
    </script>
</body>
</html>
)rawliteral";

        request->send(200, "text/html", html);
    });

    server.on("/sensor-data", HTTP_GET, handleSensorData);

server.on("/update-city", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("city")) {
        CITY = request->getParam("city")->value();
        // URL-encode the city name before using in API URLs
        String encodedCity = urlEncode(CITY);
        API_URL = "http://api.openweathermap.org/data/2.5/weather?q=" + encodedCity + "&appid=" + API_KEY + "&units=metric";
        FORECAST_URL = "http://api.openweathermap.org/data/2.5/forecast?q=" + encodedCity + "&appid=" + API_KEY + "&units=metric";
        updateWeatherData();
        request->send(200, "text/plain", "City updated to: " + CITY);
    } else {
        request->send(400, "text/plain", "Missing city parameter");
    }
});

server.on("/toggle-pump", HTTP_GET, [](AsyncWebServerRequest *request) {
    unsigned long currentTime = millis();

    // Debounce logic
    if (currentTime - lastToggleTime < debounceDelay) {
        request->send(429, "text/plain", "Too many requests. Please wait.");
        return;
    }

    lastToggleTime = currentTime;

    // Toggle the pump state
    pumpState = !pumpState;

    // Control the relay
    if (pumpState) {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW);  // Active-Low Relay ON
        Serial.println("Pump turned ON");
    } else {
        pinMode(RELAY_PIN, INPUT_PULLUP);  // Set pin to INPUT_PULLUP mode
        Serial.println("Pump turned OFF");
    }

    // Send the updated state back to the client
    request->send(200, "text/plain", pumpState ? "ON" : "OFF");
});

    server.begin();
}