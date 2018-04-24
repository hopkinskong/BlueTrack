module.exports = {
	// GET /logout
	showMainPage: function(req, res) {
		if(res.locals.loggedIn === false) { // Restricted area
			res.redirect('/');
			return;
		}
		res.render('main');
	}, 
	showMainDashboard: function(req, res) {
		if(res.locals.loggedIn === false) { // Restricted area
			res.redirect('/');
			return;
		}
		res.render('main/dashboard');
	}, 
	showMainSniffers: function(req, res) {
		if(res.locals.loggedIn === false) { // Restricted area
			res.redirect('/');
			return;
		}
		res.render('main/sniffers');
	}, 
	showMainBLEDevices: function(req, res) {
		if(res.locals.loggedIn === false) { // Restricted area
			res.redirect('/');
			return;
		}
		res.render('main/ble_devices');
	}, 
	showMainTracking: function(req, res) {
		if(res.locals.loggedIn === false) { // Restricted area
			res.redirect('/');
			return;
		}
		res.render('main/tracking');
	}, 
	showMainPhotos: function(req, res) {
		if(res.locals.loggedIn === false) { // Restricted area
			res.redirect('/');
			return;
		}
		res.render('main/photos');
	},
	showMainSettings: function(req, res) {
		if(res.locals.loggedIn === false) { // Restricted area
			res.redirect('/');
			return;
		}
		res.render('main/settings');
	}, 
}