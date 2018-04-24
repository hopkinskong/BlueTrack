var btConsole = {
	init: function() {
		btConsole.container = $("#console");
		btConsole.textContainer = $("#console_texts");
		btConsole.clear();
		btConsole.textContainer.html("[" + util.getTimeString() + "] BlueTrack WebApp Console Initialized.");
	}, 
	info: function(text) {
		btConsole.textContainer.append("<br />[" + util.getTimeString() + "] " + text);
		btConsole.container[0].scrollTop=btConsole.container[0].scrollHeight;
		
	}, 
	error: function(text) {
		btConsole.textContainer.append("<br /><span class=\"error\">[" + util.getTimeString() + "] " + text + "</span>");
		btConsole.container[0].scrollTop=btConsole.container[0].scrollHeight;
	}, 
	log: function(text) {
		btConsole.info(text)
	}, 
	clear: function() {
		btConsole.textContainer.empty();
	}
};