<?php
include('config.php');

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $username = $_POST['username'];
    $email = $_POST['email'];
    $password = $_POST['password'];

    // Password validation
    if (!preg_match('/^(?=.*[A-Z])(?=.*[a-z])(?=.*\d)(?=.*[\W_]).{8,12}$/', $password)) {
        header("Location: register.php?error=invalid_password");
        exit();
    }

    // Check if first user
    $result = $conn->query("SELECT COUNT(*) AS count FROM users");
    $row = $result->fetch_assoc();
    $is_first = $row['count'] == 0;

    $is_admin = $is_first ? 1 : 0;
    $status = $is_first ? 'approved' : 'pending';
    $hashed_password = password_hash($password, PASSWORD_DEFAULT);

    $stmt = $conn->prepare("INSERT INTO users (username, password, email, is_admin, status) VALUES (?, ?, ?, ?, ?)");
    $stmt->bind_param("sssss", $username, $hashed_password, $email, $is_admin, $status);

    if ($stmt->execute()) {
        if ($is_first) {
            header("Location: login.php?registration=success");
        } else {
            header("Location: login.php?pending=approval");
        }
    } else {
        if ($stmt->errno == 1062) {
            header("Location: register.php?error=duplicate");
        } else {
            echo "Error: " . $stmt->error;
        }
    }
    $stmt->close();
}
$conn->close();
?>