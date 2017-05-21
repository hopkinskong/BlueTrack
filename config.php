<?php
$basePath = '/bluetrack'; // For AltoRouter and various things in View, do not trail with slash
$websocket_url = "ws://192.168.0.25:9999"; // WebSocket URL for the web app to connect to which communicates with the backend

// Not going to use SQL, so only crude authentication here
$auth_username = "bluetrack";
$auth_password = "1234";

$show_error_reason_in_server_errors = true; // Show error reason in server error pages

?>