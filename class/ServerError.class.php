<?php
require_once(dirname(__FILE__) . '/../config.php');
class ServerError {
    public function throwError ($code, $reason) {
        $filePath = 'views/ServerError/' . $code . '.php';
        if($GLOBALS['show_error_reason_in_server_errors'] == TRUE) $error_reason = $reason;
        if(file_exists($filePath)) {
            include_once($filePath);
        }else{
            $iseFilePath = 'views/ServerError/500.php';
            if(file_exists($iseFilePath)) {
                $this->throwError(500, 'Unable to output ServerError for status code ' . $code . ': ' . $filePath . ' not found.');
            }else{
                header( $_SERVER["SERVER_PROTOCOL"] . ' 500 Internal Server Error'); // Throw default 500 error
            }
        }
        exit();
    }
}
?>