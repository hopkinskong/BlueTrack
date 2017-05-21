<?php
require(dirname(__FILE__) . '/FormError.class.php');
class FormErrors {
    private $errors;
    private $haveError;

    public function __construct() {
        $this->errors = array();
        $this->haveError = false;
    }

    public function clear() {
        self::__construct();
    }

    public function has($name) {
        /* @var $e FormError */
        foreach($this->errors as $e) {
            if($e->fieldName == $name) {
                return true;
            }
        }
        return false;
    }

    public function getErrorMsg($name) {
        /* @var $e FormError */
        foreach($this->errors as $e) {
            if($e->fieldName == $name) {
                return $e->errorMsg;
            }
        }
        return null;
    }

    public function getMultipleErrorMsg($name) {
        $errors = array();
        /* @var $e FormError */
        foreach($this->errors as $e) {
            if($e->fieldName == $name) {
                array_push($errors, $e->errorMsg);
            }
        }
        if(sizeof($errors) > 0) {
            return $errors;
        }
        return null;
    }

    public function add($n, $e) {
        array_push($this->errors, new FormError($n, $e));
        $this->haveError=true;
    }

    public function haveError() {
        return $this->haveError;
    }
}
?>