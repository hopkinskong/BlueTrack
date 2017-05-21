<?php
require_once(dirname(__FILE__) . '/../config.php');
require_once(dirname(__FILE__) . '/../class/View.class.php');
require_once(dirname(__FILE__) . '/../func/Guard.func.php');
require_once(dirname(__FILE__) . '/../class/FormErrors.class.php');
require_once(dirname(__FILE__) . '/../func/PrintOldFormValue.func.php');
class LogoutController {
    public static function logout() {
        // This show the normal login page
        Guard('user', ''); // Redirect to index if not user

        $_SESSION['user_role'] = 'guest'; // Reset role

        $logoutView = new View('logout');
        $logoutView->render();
    }
}
?>