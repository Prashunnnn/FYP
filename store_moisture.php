<?php
// Database Configuration
$servername = "localhost";
$username = "root";  // Default XAMPP username
$password = "";  // No password in XAMPP
$dbname = "moisture_db";  // Ensure this matches your database

// Create Connection with a unique variable name
$moisture_conn = new mysqli($servername, $username, $password, $dbname);

// Check Connection
if ($moisture_conn->connect_error) {
    die("Database Connection Failed: " . $moisture_conn->connect_error);
}

// Accept Data from GET and POST Requests
if (isset($_POST['moisture']) && isset($_POST['temperature']) && isset($_POST['humidity'])) {
    $moisture = intval($_POST['moisture']);
    $temperature = floatval($_POST['temperature']);
    $humidity = floatval($_POST['humidity']);
} elseif (isset($_GET['moisture']) && isset($_GET['temperature']) && isset($_GET['humidity'])) {
    $moisture = intval($_GET['moisture']);
    $temperature = floatval($_GET['temperature']);
    $humidity = floatval($_GET['humidity']);
} else {
    die("No data received!");
}

// Store Data in Database
$sql = "INSERT INTO moisture_data (moisture_level, temperature, humidity) VALUES ('$moisture', '$temperature', '$humidity')";
if ($moisture_conn->query($sql) === TRUE) {
    echo "Data Stored Successfully";
} else {
    echo "Error: " . $moisture_conn->error;
}

// Close Connection
$moisture_conn->close();
?>