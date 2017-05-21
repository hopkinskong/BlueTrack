<?php
require_once(dirname(__FILE__) . '/../func/IncludeCSS.func.php');
require_once(dirname(__FILE__) . '/../func/IncludeJS.func.php');
require_once(dirname(__FILE__) . '/ServerError.class.php');
class View {
    private $viewName, $viewVars, $tplName, $customJS;

    /**
     * View constructor. Note that $viewName should match with views/$viewName.php
     * @param string $viewName: Name of the view
     * @param string $tplName: Template name
     */
    function __construct($viewName, $tplName="default") {
        $this->viewName=$viewName;
        $this->viewVars=array();
        $this->customJS=array();
        $this->tplName=$tplName;

        if(!is_file(dirname(__FILE__) . '/../views/' . $this->viewName . '.view.php')) {
            (new ServerError())->throwError(500, 'Unable to load view: ' . $viewName);
        }

        if(!is_file(dirname(__FILE__) . '/../views/template/' . $this->tplName . '.tpl.php')) {
            (new ServerError())->throwError(500, 'Unable to load template: ' . $tplName);
        }
    }

    /**
     * Render View to clients. Pass $vars as variables key pair to let them to be used in the view file
     * @param array $vars: var key and value pairs
     */
    function render($vars = array()) {
        $currentView = $this;
        $customJS = $this->customJS;
        extract(array_merge($this->viewVars, $vars));
        include(dirname(__FILE__) . '/../views/template/' . $this->tplName . '.tpl.php');
    }

    /**
     * Add a single variable to the view
     * @param string $varName var name
     * @param $varValue: Var value
     */
    function addVar($varName, $varValue) {
        $this->viewVars[(string)$varName]=$varValue;
    }

    /**
     * @param string $js: Custom js file, note that ".js" and "js/" is not needed, only include the file name.
     */
    function addCustomJS($js) {
        array_push($this->customJS, $js);
    }

    /*
     * Show the main content in the template
     */
    /** Show the main content, to be called in template only
     * @param array $vars: Variables to be used in the view, in form of key and value pairs
     */
    function showMainContent($vars=array()) {
        extract($vars);
        include(dirname(__FILE__) . '/../views/' . $this->viewName . '.view.php');
    }

    /**
     * Get current view name
     * @return string of current view name
     */
    function getViewName() {
        return $this->viewName;
    }
}
?>