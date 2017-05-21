<?php
/* @var $currentView View */
/* @var $customJS array */
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>BlueTrack</title>
    <link rel="stylesheet" href="<?php echo $GLOBALS['basePath']; ?>/css/bootstrap.min.css">
    <link rel="stylesheet" href="<?php echo $GLOBALS['basePath']; ?>/css/common.css">
    <?php IncludeCSS('css/'.$currentView->getViewName().'.css'); // Include CSS if exists  ?>

    <script src="<?php echo $GLOBALS['basePath']; ?>/js/jquery.min.js"></script>
    <script src="<?php echo $GLOBALS['basePath']; ?>/js/bootstrap.min.js"></script>
    <?php foreach($customJS as $js) { IncludeJS('js/'.$js.'.js'); } ?>
    <?php IncludeJS('js/'.$currentView->getViewName().'.js'); // Include JS if exists  ?>
</head>
<body>
<div class="container">
    <?php $currentView->showMainContent(compact(array_keys(get_defined_vars()))); ?>
</div>
</body>
</html>