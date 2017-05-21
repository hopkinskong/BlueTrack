<?php
class FormError {
    public function __construct($name, $msg) {
        $this->fieldName = $name;
        $this->errorMsg = $msg;
    }

    public $fieldName;
    public $errorMsg;
}
?>