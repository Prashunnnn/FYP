<?php
include('config.php');

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $username = $_POST['username'];
    $password = $_POST['password'];

    $stmt = $conn->prepare("SELECT id, password, is_admin, status FROM users WHERE username = ?");
    $stmt->bind_param("s", $username);
    $stmt->execute();
    $result = $stmt->get_result();
    
    if ($result->num_rows == 1) {
        $user = $result->fetch_assoc();
        if ($user['status'] !== 'approved') {
            header("Location: login.php?error=pending");
            exit();
        }
        if (password_verify($password, $user['password'])) {
            $_SESSION['user_id'] = $user['id'];
            $_SESSION['username'] = $username;
            $_SESSION['is_admin'] = $user['is_admin'];
            $_SESSION['status'] = $user['status'];
            header("Location: dashboard.php");
            exit();
        }
    }
    header("Location: login.php?error=1");
}
$conn->close();
?>