<?php
require("config.php");
require("class/ServerError.class.php");
require("vendor/autoload.php");

session_start();

if(!isset($_SESSION['user_role'])) {
    $_SESSION['user_role'] = 'guest'; // Assign role to "guest" as no valid session available
}

$router = new AltoRouter();
$router->setBasePath($basePath);

// Route definitions
$router->map('GET', '/', 'IndexLoginController@showIndexLogin');
$router->map('POST', '/', 'IndexLoginController@processIndexLogin');
$router->map('GET', '/dashboard', 'DashboardController@showDashboard');
$router->map('GET', '/logout', 'LogoutController@logout');

$match = $router->match();

if(!$match) { // No match, which means the user is browsing a non-defined page
    (new ServerError())->throwError(404, 'Path not found');
}else{
    // There is a match
    if(is_callable($match['target'])) { // We are passing a anonymous function/closure
        call_user_func_array($match['target'], $match['params']); // Execute the anonymous function
    }else{
        // Test the syntax: "controller_name@function"
        list($controllerClassName, $methodName) = explode('@', $match['target']);
        if((@include_once("controllers/" . $controllerClassName . ".class.php")) == TRUE) {
            if (is_callable(array($controllerClassName, $methodName))) {
                call_user_func_array(array($controllerClassName, $methodName), ($match['params']));
            } else {
                // Internal error
                // We have defined a correct route but it is not callable
                (new ServerError())->throwError(500, 'Route not callable');
            }
        }else{
            // Internal error
            // We have defined a correct route but the controller is not includible (and hence not callable)
            (new ServerError())->throwError(500, 'Controller not includible');
        }
    }
}
?>