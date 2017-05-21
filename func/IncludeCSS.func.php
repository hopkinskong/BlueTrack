<?php
    function IncludeCSS($filename) {
        if(is_file($filename)) {
            echo '<link rel="stylesheet" href="' . $filename . '">';
        }
    }
?>