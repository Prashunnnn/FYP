<?php include('config.php'); ?>
<!DOCTYPE html>
<html>
<head>
    <title>Login</title>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <style>
        @media (max-width: 576px) {
            .container {
                padding: 15px;
                margin-top: 20px !important;
            }
            .card {
                border-radius: 0;
                box-shadow: none;
            }
            .form-control {
                font-size: 16px; /* Better for mobile zoom */
                height: 50px;
            }
            .btn {
                padding: 12px;
                font-size: 16px;
            }
            h2 {
                font-size: 1.75rem;
            }
        }
        body {
            min-height: 100vh;
            display: flex;
            align-items: center;
        }
    </style>
</head>
<body class="bg-light">
    <div class="container mt-5 px-3" style="max-width: 500px; width: 100%;">
        <div class="card shadow-sm">
            <div class="card-body p-4">
                <h2 class="text-center mb-4">🔒 Login</h2>
                <?php if(isset($_GET['error'])): ?>
                    <div class="alert alert-danger py-2">⚠️ Invalid credentials!</div>
                <?php endif; ?>
                <form action="login_process.php" method="POST">
                    <div class="mb-3">
                        <label class="form-label fw-medium">Username</label>
                        <input type="text" name="username" class="form-control form-control-lg" 
                               placeholder="Enter username" required>
                    </div>
                    <div class="mb-4">
                        <label class="form-label fw-medium">Password</label>
                        <input type="password" name="password" class="form-control form-control-lg" 
                               placeholder="Enter password" required>
                    </div>
                    <button type="submit" class="btn btn-primary btn-lg w-100 mb-3">
                        Sign In
                    </button>
                    <div class="text-center text-muted mt-3">
                        Don't have an account?<br>
                        <a href="register.php" class="btn btn-link p-0">Create new account</a>
                    </div>
                </form>
            </div>
        </div>
    </div>
    
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
</body>
</html>