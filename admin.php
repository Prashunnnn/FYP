<?php
include('config.php');

// Authorization check
if (!isset($_SESSION['user_id']) || !$_SESSION['is_admin']) {
    header("Location: login.php");
    exit();
}

// Handle user actions
if (isset($_GET['action']) && isset($_GET['id'])) {
    $user_id = intval($_GET['id']);
    $status = $_GET['action'] === 'approve' ? 'approved' : 'denied';
    
    $stmt = $conn->prepare("UPDATE users SET status = ? WHERE id = ?");
    $stmt->bind_param("si", $status, $user_id);
    $stmt->execute();
    $stmt->close();
}

// Fetch pending users
$stmt = $conn->prepare("SELECT id, username, email FROM users WHERE status = 'pending'");
$stmt->execute();
$pending_users = $stmt->get_result();
?>

<!DOCTYPE html>
<html>
<head>
    <title>Admin Panel</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body class="bg-light">
    <div class="container mt-5">
        <div class="card shadow">
            <div class="card-header bg-primary text-white">
                <h3 class="mb-0">Pending User Requests</h3>
            </div>
            <div class="card-body">
                <?php if ($pending_users->num_rows === 0): ?>
                    <div class="alert alert-info">No pending requests!</div>
                <?php else: ?>
                    <div class="table-responsive">
                        <table class="table table-hover">
                            <thead>
                                <tr>
                                    <th>Username</th>
                                    <th>Email</th>
                                    <th>Actions</th>
                                </tr>
                            </thead>
                            <tbody>
                                <?php while ($user = $pending_users->fetch_assoc()): ?>
                                <tr>
                                    <td><?= htmlspecialchars($user['username']) ?></td>
                                    <td><?= htmlspecialchars($user['email']) ?></td>
                                    <td>
                                        <a href="admin.php?action=approve&id=<?= $user['id'] ?>" class="btn btn-sm btn-success">Approve</a>
                                        <a href="admin.php?action=deny&id=<?= $user['id'] ?>" class="btn btn-sm btn-danger">Deny</a>
                                    </td>
                                </tr>
                                <?php endwhile; ?>
                            </tbody>
                        </table>
                    </div>
                <?php endif; ?>
                <a href="dashboard.php" class="btn btn-secondary">Back to Dashboard</a>
            </div>
        </div>
    </div>
</body>
</html>