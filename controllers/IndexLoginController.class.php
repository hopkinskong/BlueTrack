<?php
require_once(dirname(__FILE__) . '/../config.php');
require_once(dirname(__FILE__) . '/../class/View.class.php');
require_once(dirname(__FILE__) . '/../func/Guard.func.php');
require_once(dirname(__FILE__) . '/../class/FormErrors.class.php');
require_once(dirname(__FILE__) . '/../func/PrintOldFormValue.func.php');
class IndexLoginController {
    public static function showIndexLogin() {
        // This show the normal login page
        Guard('guest', 'dashboard'); // Redirect to 'dashboard' if not guest

        $loginView = new View('index');
        $loginView->addVar('form_errors', new FormErrors());
        $loginView->render();
    }

    public static function processIndexLogin() {
        Guard('guest', 'dashboard'); // Redirect to 'dashboard' if not guest

        $loginView = new View('index');
        $formErrors = new FormErrors();

        if(!isset($_POST['username'])) {
            $formErrors->add('username', 'Please enter the username');
        }
        if(!isset($_POST['password'])) {
            $formErrors->add('password', 'Please enter the password');
        }

        if(!$formErrors->haveError()) {
            if(strlen($_POST['username']) == 0) {
                $formErrors->add('username', 'Please enter the username');
            }
            if(strlen($_POST['password']) == 0) {
                $formErrors->add('password', 'Please enter the password');
            }

            if(!$formErrors->haveError()) {
                if($_POST['username'] == $GLOBALS['auth_username'] && $_POST['password'] == $GLOBALS['auth_password']) {
                    // Authenticated
                    $_SESSION['user_role'] = 'user';
                    header("Location: " . $GLOBALS['basePath'] . "/dashboard");
                }else{
                    $formErrors->add('username', ''); // Just show the red border
                    $formErrors->add('password', 'Incorrect username and/or password.');
                }
            }
        }
        $loginView->addVar('form_errors', $formErrors);
        $loginView->render();
    }
}
?>