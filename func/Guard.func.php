<?php
require_once(dirname(__FILE__) . '/../config.php');

function Guard($required_role, $failure_redirect_page) {
    global $basePath;
    $current_role = 'guest';
    if(isset($_SESSION['user_role'])) {
        $current_role = $_SESSION['user_role'];
    }
    if ($current_role != $required_role) {
        // The current user is not authorized to view this page
        if (!empty($failure_redirect_page)) {
            header("Location: $basePath/$failure_redirect_page"); // Redirect the user to $target_page
        } else {
            header("Location: $basePath"); // Redirect the user to base path (index page)
        }
    }
}

?>