
<?php include('config.php'); ?>
<!DOCTYPE html>
<html>
<head>
    <title>Register</title>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
    <style>
      @media (max-width: 576px) {
            .container {
                padding: 15px;
                margin-top: 1rem !important;
            }
            .card {
                border-radius: 0;
                box-shadow: none;
            }
            h2 {
                font-size: 1.75rem;
            }
            .form-control {
                font-size: 16px;
                height: 50px;
            }
            .btn {
                padding: 14px;
                font-size: 16px;
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
                <h2 class="text-center mb-4">📝 Create Account</h2>
                <?php if(isset($_GET['error']) && $_GET['error'] == 'invalid_password'): ?>
                    <div class="alert alert-danger py-2">⚠️ Password must be 8-12 characters, include uppercase, lowercase, number, and special character.</div>
                <?php endif; ?>
                <form action="register_process.php" method="POST">
                    <div class="mb-3">
                        <label class="form-label fw-medium">Username</label>
                        <input type="text" name="username" class="form-control form-control-lg" 
                               placeholder="Enter username" required>
                    </div>
                    <div class="mb-3">
                        <label class="form-label fw-medium">Email</label>
                        <input type="email" name="email" class="form-control form-control-lg" 
                               placeholder="Enter email" required>
                    </div>
                    <div class="mb-4">
                        <label class="form-label fw-medium">Password</label>
                        <input type="password" name="password" class="form-control form-control-lg" 
                               placeholder="Create password" required
                               pattern="^(?=.*[A-Z])(?=.*[a-z])(?=.*\d)(?=.*[\W_]).{8,12}$"
                               title="Must contain: 8-12 chars, 1 uppercase, 1 lowercase, 1 number, 1 special character">
                    </div>
                    <button type="submit" class="btn btn-primary btn-lg w-100 mb-3">
                        Register Now
                    </button>
                    <div class="text-center text-muted mt-3">
                        Already have an account?<br>
                        <a href="login.php" class="btn btn-link p-0">Login here</a>
                    </div>
                </form>
            </div>
        </div>
    </div>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
</body>
</html>