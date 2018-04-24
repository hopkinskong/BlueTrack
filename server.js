var express = require('express');
var app = express();
var expressWs = require('express-ws')(app);
var bodyParser = require('body-parser');
var multer = require('multer');
var upload = multer(); 
var session = require('express-session');
var cookieParser = require('cookie-parser');
var helmet = require('helmet');

var config = require('config');

app.set('view engine', 'pug');
app.set('views','./views');

app.use(helmet());
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true })); 
app.use(upload.array());
app.use(cookieParser());
app.use(session({
	secret: "bluetrack_flO5Gk03s6k4", 
	name: 'bluetrack_webapp_sessid', 
	resave: true, 
	saveUninitialized: false
	})
);


// Controller defines
var controllerPathPrefix = __dirname + '/controllers/';
var loginController = require(controllerPathPrefix + 'login.controller');
var logoutController = require(controllerPathPrefix + 'logout.controller');
var mainController = require(controllerPathPrefix + 'main.controller');
var wsController = require(controllerPathPrefix + 'ws.controller');

// Public folder
app.use(express.static(__dirname + '/public'));

// Middleware
app.all('*', function(req, res, next) {
	res.locals.OldFormValue = req.body; // Previously submitted form values
	res.locals.FormErrors = {}; // Form errors
	if(req.session.loggedIn === true) {
		res.locals.loggedIn = true;
		res.locals.loggedUser = req.session.loggedUser;
	}else{
		res.locals.loggedIn = false;
	}
	next();
});

// Index redirection
app.get('/', function(req, res) {
	res.redirect('/login');
});

// Login Routes
app.get('/login', loginController.showLoginPage);
app.post('/login', loginController.processLogin);

// Logout route
app.get('/logout', logoutController.logout);

// Main
app.get('/main', mainController.showMainPage);

// Main Pages
app.get('/main/dashboard', mainController.showMainDashboard);
app.get('/main/sniffers', mainController.showMainSniffers);
app.get('/main/ble_devices', mainController.showMainBLEDevices);
app.get('/main/tracking', mainController.showMainTracking);
app.get('/main/photos', mainController.showMainPhotos);
app.get('/main/settings', mainController.showMainSettings);

app.ws('/backend', wsController.websocket);

app.listen(config.get("app.app_port"));

console.info("BlueTrack WebApp listening at port: " + config.get("app.app_port"));


