#include <DHT.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> 

// Wi-Fi Credentials
const char* ssid = "prashun";
const char* password = "12345678";
const char* serverName = "http://192.168.1.132/moisture_project/store_moisture.php"; //cmd->ipconfig->enter your local ip

// Weather API
String API_KEY = "92325fb9f39650118b62df5c4363b7a5";
String CITY = "Kathmandu";
String API_URL = "http://api.openweathermap.org/data/2.5/weather?q=" + CITY + "&appid=" + API_KEY + "&units=metric";
String FORECAST_URL = "http://api.openweathermap.org/data/2.5/forecast?q=" + CITY + "&appid=" + API_KEY + "&units=metric";

// Telegram alert bot
const char* telegramBotToken = "8178345756:AAEvZv-38_BJGtn9SB4ZLRjv9Kj-m2jCXZs"; // botfather telegram, to create new bots
const char* telegramChatID = "5784424361";

// Web Server
AsyncWebServer server(80);

// Pins
#define SOIL_SENSOR_PIN 34
#define RELAY_PIN 23
#define BUTTON_PIN 18  // Optional manual button
#define DHTPIN 22   // DHT Sensor Pin
#define DHTTYPE DHT11  // DHT11 or DHT22

DHT dht(DHTPIN, DHTTYPE);

const int threshold = 40; // Moisture threshold for alert
unsigned long lastSendTime = 0;
const long sendInterval = 30000; // 10 seconds
unsigned long lastToggleTime = 0;
const unsigned long debounceDelay = 1000;  // 1-second debounce delay
bool onlineMode = false;
bool pumpState = false;
bool alertSent = false;
int moisturePercent = 0;
float temp = 0.0;
float humidity = 0.0;

// Function to Fetch Weather Data
String getWeatherData(String city) {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + API_KEY + "&units=metric";
    http.begin(url);
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

// Function to fetch forecasted data
String getForecastData(String city) {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "&appid=" + API_KEY + "&units=metric";
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        String payload = http.getString();
        http.end();
        return payload;
    } else {
        Serial.println("Error Fetching Forecast Data");
        http.end();
        return "{}";
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
        body {
            background: #1E3C72;
            text-align: center;
            padding: 20px;
            color: #fff;
            font-family: 'Poppins', sans-serif;
        }
        .card {
            border-radius: 15px;
            backdrop-filter: blur(20px);
            background: rgba(255, 255, 255, 0.15);
            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
            padding: 20px;
            color: #fff;
            transition: transform 0.3s, box-shadow 0.3s;
        }
        .card:hover {
            transform: scale(1.03);
            box-shadow: 0 6px 15px rgba(0, 0, 0, 0.4);
        }
        .pump-box {
            background: rgba(0, 0, 0, 0.2);
            padding: 15px;
            border-radius: 10px;
            display: inline-block;
            margin-top: 20px;
        }
        .btn-toggle {
            background: #28a745;
            color: white;
            border: none;
            padding: 12px 24px;
            font-size: 1.2em;
            border-radius: 10px;
            transition: background 0.3s ease-in-out;
        }
        .btn-toggle:hover {
            background: #218838;
        }
        
        #forecast-table {
            width: 100%;
            table-layout: auto; 
            word-wrap: break-word;
            border-collapse: collapse; 
        }

        #forecast-table th {
            background: rgba(0, 0, 0, 0.4);
            font-weight: bold;
            color: #ffffff;
            font-size: 1em; /* Reduce font size */
            text-transform: uppercase;
            padding: 10px;
        }

        #forecast-table td {
            padding: 8px;
            font-size: 0.9em; 
            text-align: center;
            white-space: nowrap; 
            overflow: hidden;
            text-overflow: ellipsis;
        }

        #forecast-table tbody tr:nth-child(even) {
            background: rgba(255, 255, 255, 0.1);
        }

        #forecast-table tbody tr:hover {
            background: rgba(255, 255, 255, 0.2);
            transform: scale(1.01);
            transition: all 0.3s ease-in-out;
        }

        @media (max-width: 576px) {
            #forecast-table th, #forecast-table td {
                font-size: 0.85em; /* Further reduce font size for mobile */
                padding: 6px;
            }
            .card {
                padding: 15px;
            }
            .table-responsive {
                padding: 10px;
            }
        }

        .card {
            border-radius: 18px;
            background: rgba(255, 255, 255, 0.15);
            box-shadow: 0 6px 15px rgba(0, 0, 0, 0.4);
            backdrop-filter: blur(15px);
        }

        .table-responsive {
            border-radius: 12px;
            background: rgba(255, 255, 255, 0.1);
            padding: 15px;
            overflow-x: auto;
            display: block;
            width: 100%;
        }

        .city-input {
            display: flex;
            justify-content: center;
            align-items: center;
            gap: 10px;
            flex-wrap: wrap;
            margin-bottom: 20px;
        }

        .city-input input {
            padding: 10px;
            border-radius: 5px;
            border: none;
            width: 200px;
            justify-content: center;
            align-items: center;
        }

        .city-input button {
            padding: 10px 20px;
            border-radius: 5px;
            border: none;
            background: #28a745;
            color: white;
            cursor: pointer;
        }

        .city-input button:hover {
            background: #218838;
        }
    </style>
</head>
<body>
    <h1 class="mb-4">🌳 Smart Irrigation System</h1>

    <div class="container">
        <div class="city-input">
            <input type="text" id="city-input" placeholder="Enter City Name">
            <button onclick="updateCity()">Update City</button>
        </div>

        <div class="row justify-content-center g-3">
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
        
        <div class="row justify-content-center g-3 mt-3">
            <div class="col-md-6">
                <div class="card">
                    <h3>🌦️ Live Weather Information</h3>
                    <p><strong>Pressure:</strong> <span id="pressure">--</span> hPa</p>
                    <p><strong>Condition:</strong> <span id="condition">Loading...</span></p>
                    <p><strong>Wind Speed:</strong> <span id="wind_speed">--</span> km/h</p>
                </div>
            </div>
        </div>

        <div class="row justify-content-center g-3 mt-3">
            <div class="col-md-8">
                <div class="card">
                    <h3>☁️ Weather Forecast </h3>
                    <div class="table-responsive">
                        <table class="table table-dark table-hover" id="forecast-table">
                            <thead>
                                <tr>
                                    <th>Time</th>
                                    <th>Temperature</th>
                                    <th>Condition</th>
                                </tr>
                            </thead>
                            <tbody>
                                <!-- Forecast data will be dynamically injected here -->
                            </tbody>
                        </table>
                    </div>
                </div>
            </div>
        </div>

        <div class="pump-box">
            <p class="pump-status">⚡ Pump Status: <span id="pump-status" class="text-danger">OFF</span></p>
            <button id="toggle-button" class="btn btn-toggle">Toggle Pump</button>
        </div>

        <script>
            function fetchData() {
                fetch('/sensor-data')
                    .then(response => response.json())
                    .then(data => {
                        updateSensorData(data);
                        updateForecastTable(data.forecast);
                    })
                    .catch(error => console.error("Error fetching data:", error));
            }

            function updateCity() {
              const city = document.getElementById('city-input').value;
              fetch('/update-city?city=' + city)
                  .then(response => response.text())
                  .then(data => {
                      console.log(data);
                      fetchData(); // Ensure data updates after city change
        })
        .catch(error => console.error("Error updating city:", error));
        }

            function updateSensorData(data) {
                document.getElementById('moisture').innerText = `${data.moisture}%`;
                document.getElementById('temperature').innerText = `${data.temperature}°C`;
                document.getElementById('humidity').innerText = `${data.humidity}%`;
                document.getElementById('pressure').innerText = `${data.pressure}`;
                document.getElementById('condition').innerText = data.condition;
                document.getElementById("wind_speed").innerText = `${(data.wind_speed * 3.6).toFixed(1)}`;

                let pumpStatus = document.getElementById("pump-status");
                pumpStatus.innerText = data.pump ? "ON" : "OFF";
                pumpStatus.className = data.pump ? "text-success fw-bold" : "text-danger fw-bold";
            }

            function updateForecastTable(forecast) {
                const forecastTable = document.querySelector("#forecast-table tbody");
                forecastTable.innerHTML = ''; // Clear previous data

                if (forecast && forecast.length > 0) {
                    forecast.forEach(forecastEntry => {
                        let row = document.createElement("tr");

                        // Date & Time Column
                        let dateTimeCell = document.createElement("td");
                        dateTimeCell.innerText = formatDateTime(forecastEntry.time);
                        row.appendChild(dateTimeCell);

                        // Temperature Column
                        let tempCell = document.createElement("td");
                        tempCell.innerText = `${forecastEntry.temp.toFixed(1)}°C`;
                        row.appendChild(tempCell);

                        // Condition Column with Icon
                        let conditionCell = document.createElement("td");
                        let iconSpan = document.createElement("span");
                        iconSpan.innerHTML = getWeatherIcon(forecastEntry.condition) + " ";
                        let textNode = document.createTextNode(forecastEntry.condition);
                        conditionCell.appendChild(iconSpan);
                        conditionCell.appendChild(textNode);
                        row.appendChild(conditionCell);

                        forecastTable.appendChild(row);
                    });
                } else {
                    let row = document.createElement("tr");
                    row.innerHTML = `<td colspan="3" class="text-center text-warning">No forecast data available</td>`;
                    forecastTable.appendChild(row);
                }
            }

            function formatDateTime(timestamp) {
                let date = new Date(timestamp);
                let options = { weekday: 'short', month: 'short', day: 'numeric' };
                let formattedDate = date.toLocaleDateString('en-US', options);
                let hours = date.getHours();
                let minutes = date.getMinutes();
                let ampm = hours >= 12 ? "PM" : "AM";
                hours = hours % 12 || 12;
                let formattedTime = `${hours}:${minutes.toString().padStart(2, '0')} ${ampm}`;
                return `${formattedDate}, ${formattedTime}`;
            }

            function getWeatherIcon(condition) {
                condition = condition.toLowerCase();
                if (condition.includes("clear")) return "☀️";
                if (condition.includes("cloud")) return "☁️";
                if (condition.includes("rain")) return "🌧️";
                if (condition.includes("storm")) return "⛈️";
                if (condition.includes("snow")) return "❄️";
                if (condition.includes("haze") || condition.includes("fog")) return "🌫️";
                return "🌍";
            }

            document.getElementById("toggle-button").addEventListener("click", function() {
                fetch("/toggle-pump")
                    .then(response => response.text())
                    .then(status => {
                        document.getElementById("pump-status").innerText = status;
                        document.getElementById("pump-status").className = status === "ON" ? "text-success" : "text-danger";
                    })
                    .catch(error => console.error("Error toggling pump:", error));
            });

            setInterval(fetchData, 3000);
        </script>
    </div>
</body>
</html>
)rawliteral";

        request->send(200, "text/html", html);
    });

    server.on("/sensor-data", HTTP_GET, [](AsyncWebServerRequest *request){
        String weatherData = getWeatherData(CITY);  // Fetch weather info
        String forecastData = getForecastData(CITY); // Fetch forecasted info

        Serial.println("🌦️ Weather Data: " + weatherData);
        Serial.println("☁️ Forecast Data: " + forecastData);

        // Create JSON response object
        DynamicJsonDocument json(2048);  

        // Parse current weather JSON
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, weatherData);
        
        int pressure = 0;
        float windSpeed = 0;
        String condition = "Unavailable";

        if (!error) {
            pressure = doc["main"]["pressure"] | 0;
            windSpeed = doc["wind"]["speed"] | 0.0;  
            condition = doc["weather"][0]["description"].as<String>();
        } else {
            Serial.println("❌ Error parsing Weather JSON");
        }

        // Read and convert soil moisture
        int moisture = analogRead(SOIL_SENSOR_PIN);
        moisture = constrain(moisture, 2500, 4095);
        int moisturePercent = map(moisture, 4095, 2500, 0, 100);
        delay(500);
        
        float temp = dht.readTemperature();
        float humidity = dht.readHumidity();

        // Parse forecast JSON
        DynamicJsonDocument forecastDoc(4096);
        DeserializationError forecastError = deserializeJson(forecastDoc, forecastData);
        
        JsonArray forecastJson = json.createNestedArray("forecast");

        if (!forecastError) {
            JsonArray forecastArray = forecastDoc["list"].as<JsonArray>();

            if (!forecastArray.isNull()) {
                Serial.println("✅ Successfully parsed forecast data.");
                for (int i = 0; i < 3 && i < forecastArray.size(); i++) {
                    JsonObject forecastEntry = forecastArray[i];

                    String time = forecastEntry["dt_txt"].as<String>();
                    float forecastTemp = forecastEntry["main"]["temp"] | 0.0;
                    String forecastCondition = forecastEntry["weather"][0]["description"].as<String>();

                    JsonObject forecastObj = forecastJson.createNestedObject();
                    forecastObj["time"] = time;
                    forecastObj["temp"] = forecastTemp;
                    forecastObj["condition"] = forecastCondition;
                }
            } else {
                Serial.println("⚠️ Forecast list is empty or missing!");
            }
        } else {
            Serial.println("❌ Error Parsing Forecast JSON");
        }

        // Prepare JSON response
        json["moisture"] = moisturePercent;
        json["temperature"] = temp;
        json["humidity"] = humidity;
        json["pressure"] = pressure;
        json["wind_speed"] = windSpeed;
    json["condition"] = condition;
    json["pump"] = pumpState;

    String response;
    serializeJson(json, response);
    
    request->send(200, "application/json", response);
});


server.on("/update-city", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("city")) {
        CITY = request->getParam("city")->value();
        Serial.println("City updated to: " + CITY);
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

void checkSoilMoisture();
void sendMoistureData(int moisturePercent, float temp, float humidity);
void sendAlert();

void setup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SOIL_SENSOR_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(RELAY_PIN, HIGH);

    dht.begin();

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
    unsigned long currentMillis = millis();
    
    if (WiFi.status() == WL_CONNECTED && (currentMillis - lastSendTime > sendInterval)) {
        sendMoistureData(moisturePercent, temp, humidity);
        lastSendTime = currentMillis;
    }

    if (moisturePercent < threshold && !alertSent) {
        sendAlert();
        alertSent = true;
    } else if (moisturePercent >= threshold) {
        alertSent = false;
    }
}

void checkSoilMoisture() {
    int moisture = analogRead(SOIL_SENSOR_PIN);
    moisture = constrain(moisture, 2500, 4095);
    moisturePercent = map(moisture, 4095, 2500, 0, 100);
    Serial.print("Soil Moisture: ");
    Serial.print(moisturePercent);
    Serial.println("%");

    delay(500);

    temp = dht.readTemperature();  // Read from DHT sensor
    humidity = dht.readHumidity();
    
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" deg C");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");


    if (moisturePercent < 40 && !pumpState) {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW); // Turn ON pump
        pumpState = true;
        Serial.println("Pump ON");
    } else if (moisturePercent >= 40 && pumpState) {
        pinMode(RELAY_PIN, INPUT_PULLUP);
        digitalWrite(RELAY_PIN, HIGH); // Turn OFF pump
        pumpState = false;
        Serial.println("Pump OFF");
    }
}

void sendMoistureData(int moisturePercent, float temp, float humidity) {
    WiFiClient client;
    HTTPClient http;
    
    String postData = "moisture=" + String(moisturePercent) + 
                      "&temperature=" + String(temp) + 
                      "&humidity=" + String(humidity);

    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
        Serial.println("Data Stored Successfully: " + http.getString());
    } else {
        Serial.println("Error Storing Data: " + String(httpResponseCode));
        Serial.println("Check server, database, and PHP script.");
    }

    http.end();
}

// alert using  telegram
String urlencode(String str) {
    String encodedString = "";
    char c;
    char code0;
    char code1;
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == ' ') {
            encodedString += "%20"; // Telegram API needs %20 for spaces
        } else if (isalnum(c)) {
            encodedString += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) code1 = (c & 0xf) - 10 + 'A';
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) code0 = c - 10 + 'A';
            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
        }
    }
    return encodedString;
}

void sendAlert() {
    WiFiClientSecure client;  // Use a secure client
    client.setInsecure();  // Ignore SSL certificate validation

    HTTPClient http;
    
    String message = "🚨 ALERT! Soil moisture is critically low!";
    String encodedMessage = urlencode(message);

    String alertURL = "https://api.telegram.org/bot" + String(telegramBotToken) +
                      "/sendMessage?chat_id=" + String(telegramChatID) +
                      "&text=" + encodedMessage;

    Serial.print("Telegram API Request: ");
    Serial.println(alertURL);

    http.begin(client, alertURL);  // Use the secure client
    int httpResponseCode = http.GET();

    Serial.print("Telegram HTTP Response Code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
        String response = http.getString();  // Get API response
        Serial.print("Telegram API Response: ");
        Serial.println(response);
    } else {
        Serial.println("❌ Error: Failed to send request to Telegram API.");
    }

    http.end();
}

