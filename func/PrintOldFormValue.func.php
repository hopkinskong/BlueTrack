<?php
function PrintOldFormValue($field, $default = '') {
    if(isset($_POST[$field])) {
        if(!empty($_POST[$field])) {
            echo htmlspecialchars($_POST[$field], ENT_QUOTES);
        }
    }
    echo htmlspecialchars($default, ENT_QUOTES);
}
?>