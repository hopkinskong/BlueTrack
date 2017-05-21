<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>BlueTrack | 500 Internal Server Error</title>
    <link rel="stylesheet" href="<?php echo $GLOBALS['basePath']; ?>/css/bootstrap.min.css">
    <link rel="stylesheet" href="<?php echo $GLOBALS['basePath']; ?>/css/common.css">
    <script src="<?php echo $GLOBALS['basePath']; ?>/js/jquery.min.js"></script>
    <script src="<?php echo $GLOBALS['basePath']; ?>/js/bootstrap.min.js"></script>
</head>
<body>
<div class="container">
    <div class="row vertical-offset-100">
        <div class="col-md-6 col-md-offset-3">
            <div class="panel panel-default">
                <div class="panel-heading text-center">
                    <h3 class="panel-title">500 Internal Server Error</h3>
                </div>
                <div class="panel-body text-center">
                    <p>Something went wrong, I'm sorry :(</p>

                    <?php
                    if(isset($error_reason)) echo "<p>Details: <strong>$error_reason</strong></p>";
                    ?>
                </div>
            </div>
        </div>
    </div>
</div>
</body>
</html>