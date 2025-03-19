#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>

// Wi-Fi Credentials
const char* ssid = "prashun";
const char* password = "12345678";

// Weather API
String API_KEY = "92325fb9f39650118b62df5c4363b7a5";
String CITY = "Kathmandu";
String API_URL = "http://api.openweathermap.org/data/2.5/weather?q=Kathmandu&appid=" + API_KEY + "&units=metric";

// Web Server
AsyncWebServer server(8080);

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
bool manualOverride = false;  // Global variable to track manual control
// Function to Check Soil Moisture and Auto Control Pump
void checkSoilMoisture() {

    int moisture = analogRead(SOIL_SENSOR_PIN);
    moisture = constrain(moisture, 2500, 4095);
    int moisturePercent = map(moisture, 4095, 2500, 0, 100);
    
    Serial.print("Soil Moisture: ");
    Serial.print(moisturePercent);
    Serial.println("%");

    if (moisturePercent < 40 && !pumpState) {  
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW);  // Turn ON
        pumpState = true;
        Serial.println("Pump ON");
    } 
    else if (moisturePercent >= 40 && pumpState) {  
        pinMode(RELAY_PIN, INPUT_PULLUP);
        digitalWrite(RELAY_PIN, HIGH); // Turn OFF
        pumpState = false;
        Serial.println("Pump OFF");
    }
}

// Function to Handle Web Requests
void startServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Smart Irrigation System</title>
    <!-- Bootstrap CSS -->
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <style>
        /* Simple Blue Background */
        body {
            background: #1E3C72;
            text-align: center;
            padding: 20px;
            color: #fff;
            font-family: 'Poppins', sans-serif;
        }

        /* Glassmorphism Effect for Cards */
        .card {
            border-radius: 15px;
            backdrop-filter: blur(20px);
            background: rgba(255, 255, 255, 0.15);
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
            margin: 20px auto;
            max-width: 450px;
            padding: 20px;
            color: #fff;
            transition: transform 0.3s, box-shadow 0.3s;
        }

        .card:hover {
            transform: scale(1.03);
            box-shadow: 0 6px 15px rgba(0, 0, 0, 0.4);
        }

        .moisture-card {
            background: rgba(255, 223, 87, 0.3);
        }

        .weather-card {
            background: rgba(74, 144, 226, 0.3);
        }

        /* Pump Status */
        .pump-status {
            font-size: 1.4em;
            font-weight: bold;
            margin-top: 10px;
        }

        /* Toggle Button */
        .btn-toggle {
            font-size: 1.2em;
            font-weight: bold;
            border-radius: 10px;
            background: linear-gradient(135deg, #34D399, #059669);
            border: none;
            padding: 12px 24px;
            color: white;
            transition: 0.3s ease-in-out;
        }

        .btn-toggle:hover {
            background: linear-gradient(135deg, #059669, #047857);
            transform: scale(1.05);
        }

        /* Icons */
        .emoji {
            font-size: 1.5em;
        }
    </style>
</head>
<body>

    <h1 class="mb-4">🚀 Smart Irrigation System</h1>

    <!-- Soil Moisture Card -->
    <div class="card moisture-card">
        <h3>🌱 Soil Moisture</h3>
        <p class="display-4 fw-bold" id="moisture-value">--%</p>
    </div>

    <!-- Weather Information Card -->
    <div class="card weather-card">
        <h3>🌦️ Weather Information</h3>
        <p><strong>Temperature:</strong> <span id="temperature">--</span>°C</p>
        <p><strong>Humidity:</strong> <span id="humidity">--</span>%</p>
        <p><strong>Pressure:</strong> <span id="pressure">--</span> hPa</p>
        <p><strong>Wind Speed:</strong> <span id="wind_speed">--</span> km/h</p>
        <p><strong>Wind Direction:</strong> <span id="wind_direction">--</span></p>
        <p><strong>Condition:</strong> <span id="condition">Loading...</span></p>
    </div>

    <!-- Pump Status -->
    <p class="pump-status">🚰 Pump Status: <span id="pump-status" class="text-danger">OFF</span></p>

    <!-- Toggle Pump Button -->
    <button id="toggle-button" class="btn btn-toggle">Toggle Pump</button>

    <!-- JavaScript -->
    <script>
        function convertWindDirection(degrees) {
            const directions = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"];
            return directions[Math.round(degrees / 45) % 8];
        }

        async function fetchData() {
            try {
                let response = await fetch("/sensor-data");
                let data = await response.json();

                document.getElementById("moisture-value").innerText = data.moisture + "%";
                document.getElementById("temperature").innerText = data.temperature;
                document.getElementById("humidity").innerText = data.humidity;
                document.getElementById("pressure").innerText = data.pressure;  
                document.getElementById("wind_speed").innerText = (data.wind_speed * 3.6).toFixed(1); 
                document.getElementById("wind_direction").innerText = convertWindDirection(data.wind_direction);
                document.getElementById("condition").innerText = data.condition;
                document.getElementById("pump-status").innerText = data.pump ? "ON" : "OFF";
                document.getElementById("pump-status").className = data.pump ? "text-success" : "text-danger";
            } catch (error) {
                console.error("Error fetching data:", error);
            }
        }

let isToggling = false;  // Prevents multiple simultaneous toggles

// Function to start toggling when button is pressed
function startToggling() {
    if (isToggling) return;  // Prevents multiple triggers
    isToggling = true;

    togglePump();  // Immediately toggle once

    toggleInterval = setInterval(() => {
        togglePump();  // Keep toggling every 1 second while holding
    }, 1000);
}

// Function to stop toggling when button is released
function stopToggling() {
    clearInterval(toggleInterval);
    isToggling = false;  // Allow toggling again
}

// Function to send the toggle request
function togglePump() {
    fetch("/toggle-pump")
        .then(response => response.text())
        .then(status => {
            console.log("Pump Toggled: " + status);
            document.getElementById("pump-status").innerText = status;
            document.getElementById("pump-status").className = status === "ON" ? "text-success" : "text-danger";
        })
        .catch(error => console.error("Error toggling pump:", error));
}

// Attach event listeners to the button
document.getElementById("toggle-button").addEventListener("mousedown", startToggling);
document.getElementById("toggle-button").addEventListener("mouseup", stopToggling);
document.getElementById("toggle-button").addEventListener("mouseleave", stopToggling);  // Stops toggling if mouse leaves




    setInterval(fetchData, 3000);
    </script>

    <!-- Bootstrap JS -->
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>

</body>
</html>




)rawliteral";

        request->send(200, "text/html", html);
    });

  server.on("/sensor-data", HTTP_GET, [](AsyncWebServerRequest *request){
    String weatherData = getWeatherData();  // Fetch weather info

    // Parse JSON from OpenWeather API
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, weatherData);
    
    float temp = 0;
    int humidity = 0;
    int pressure = 0;
    float windSpeed = 0;
    int windDirection = 0;
    String condition = "Unavailable";

    if (!error) {
        temp = doc["main"]["temp"];
        humidity = doc["main"]["humidity"];
        pressure = doc["main"]["pressure"];  // Extract pressure
        windSpeed = doc["wind"]["speed"];  // Wind speed in m/s
        windDirection = doc["wind"]["deg"];  // Wind direction in degrees
        condition = doc["weather"][0]["description"].as<String>();
    }

    // Read and convert soil moisture
    int moisture = analogRead(SOIL_SENSOR_PIN);
    moisture = constrain(moisture, 2500, 4095);
    int moisturePercent = map(moisture, 4095, 2500, 0, 100);

    // Create JSON response
    DynamicJsonDocument json(256);
    json["moisture"] = moisturePercent;
    json["temperature"] = temp;
    json["humidity"] = humidity;
    json["pressure"] = pressure;  // Include pressure data
    json["wind_speed"] = windSpeed;
    json["wind_direction"] = windDirection;
    json["condition"] = condition;
    json["pump"] = pumpState;

    String response;
    serializeJson(json, response);
    
    request->send(200, "application/json", response);
});


// Pump Toggle API
server.on("/toggle-pump", HTTP_GET, [](AsyncWebServerRequest *request){
    pumpState = !pumpState;  // Toggle state

    if (pumpState) {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW);  // Active-Low Relay ON
        Serial.println("Pump ON");
    } else {
        pinMode(RELAY_PIN, INPUT_PULLUP);
        digitalWrite(RELAY_PIN, HIGH); // Active-Low Relay OFF
        Serial.println("Pump OFF");
    }

    request->send(200, "text/plain", pumpState ? "ON" : "OFF");
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
    delay(1000);
}
