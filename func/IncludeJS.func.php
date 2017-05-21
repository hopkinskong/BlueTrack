<?php
function IncludeJS($filename) {
    if(is_file($filename)) {
        echo '<script src="' . $filename . '"></script> ';
    }
}
?>