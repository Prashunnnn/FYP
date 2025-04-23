<?php
include('config.php');

// Authorization check
if (!isset($_SESSION['user_id']) || $_SESSION['status'] !== 'approved') {
    header("Location: login.php");
    exit();
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>Dashboard</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
    <style>
        .iframe-container {
            position: relative;
            overflow: hidden;
            padding-top: 56.25%;
            height: calc(100vh - 150px);
        }
        
        .iframe-container iframe {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            border: 0;
        }

        @media (max-width: 768px) {
            .navbar-text {
                font-size: 0.9rem;
                margin-right: 0.5rem !important;
            }
            
            .card-header h3 {
                font-size: 1.25rem;
            }
            
            .btn-outline-light {
                padding: 0.25rem 0.5rem;
                font-size: 0.9rem;
            }
            
            .container {
                padding-left: 10px;
                padding-right: 10px;
            }
            
            .card-body {
                padding: 15px;
            }
            
            .nav-link {
                padding: 0.5rem 0.7rem;
            }
        }
    </style>
</head>
<body class="d-flex flex-column min-vh-100">
    <nav class="navbar navbar-expand-lg navbar-dark bg-dark">
        <div class="container-fluid">
            <a class="navbar-brand" href="#">Smart Irrigation</a>
            <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarNav">
                <span class="navbar-toggler-icon"></span>
            </button>
            <div class="collapse navbar-collapse" id="navbarNav">
                <div class="navbar-nav ms-auto">
                    <?php if ($_SESSION['is_admin']): ?>
                        <a class="nav-link me-3" href="admin.php">
                            <i class="bi bi-shield-lock"></i> Admin Panel
                        </a>
                    <?php endif; ?>
                    <span class="navbar-text text-white me-3">
                        <i class="bi bi-person-circle"></i> <?= htmlspecialchars($_SESSION['username']) ?>
                    </span>
                    <a href="logout.php" class="btn btn-outline-light">
                        <i class="bi bi-box-arrow-right"></i> Logout
                    </a>
                </div>
            </div>
        </div>
    </nav>

    <div class="container-fluid flex-grow-1 mt-2">
        <div class="row justify-content-center">
            <div class="col-12 col-lg-8">
                <div class="card shadow-sm">
                    <div class="card-header bg-dark text-white">
                        <h3 class="text-center mb-0">Digital Irrigation System Control Panel</h3>
                    </div>
                    <div class="card-body p-0">
                        <div class="iframe-container">
                            <iframe src="http://192.168.137.206"></iframe>
                        </div>
                    </div>
                    <div class="card-footer bg-light text-center">
                        <small class="text-muted">System Status: Connected</small>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
</body>
</html>