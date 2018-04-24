var config = require('config');

var UserExists = function(username, password) {
	var allUsers = config.get('login.credentials');
	for(let i=0; i<allUsers.length; i++) {
		if(allUsers[i][0] === username && allUsers[i][1] === password) {
			return true;
		}
	}
	return false;
}

module.exports = {
	
	// GET /login
	showLoginPage: function(req, res) {
		if(res.locals.loggedIn === true) {
			res.redirect('/main');
			return;
		}
		res.render('login');
	}, 
	
	// POST /login
	processLogin: function(req, res) {
		if(res.locals.loggedIn === true) {
			res.redirect('/main');
			return;
		}
		
		var FormErrors = res.locals.FormErrors;
		if(req.body.username.length == 0) {
			FormErrors.username = "Please enter the username";
		}
		
		if(req.body.password.length == 0) {
			FormErrors.password = "Please enter the password";
		}
		
		// Validation done
		if(Object.keys(FormErrors).length == 0) {
			// Try login now
			if(UserExists(req.body.username, req.body.password)) {
				req.session.loggedIn = true;
				req.session.loggedUser = req.body.username;
				res.redirect('/main');
				return;
			}
			FormErrors.username = "Incorrect username and/or password.";
			FormErrors.password = ""; // Just show the red border without text
		}
		res.render('login');
	}
}