module.exports = {
	// GET /logout
	logout: function(req, res) {
		if(res.locals.loggedIn === false) { // Why logout when not logged in?
			res.redirect('/');
			return;
		}
		
		res.locals.loggedIn=false;
		req.session.loggedIn=false;
		delete req.session.loggedIn;
		req.session.destroy();
		
		req.session = null;
		
		res.render('logout');
	}, 
	
}