var config = require('config');
var zmq = require('zeromq');
var zmq_sock = zmq.socket('req');

// ZMQ Connect
console.log("ZMQ RPC Address: " + config.get('app.zmq_rpc_address'));
zmq_sock.connect(config.get('app.zmq_rpc_address'));

// Clients management
var clients = [];
var findClient = function(sessID) {
	var found = 0;
	var i;
	for(i=0; i<clients.length; i++) {
		if(clients[i].sessID == sessID) {
			found = 1;
			break;
		}
	}
	
	if(found !== 1) return -1;
	return i;
}

var removeClient = function(sessID) {
	var index = findClient(sessID);
	if(index !== -1) clients.splice(index, 1);
}

var addClient = function(sessID, ws) {
	clients[clients.length] = {
		sessID: sessID, 
		ws: ws
	};
}

// ZMQ Received a message, transfer to clients
zmq_sock.on('message', function(msg) {
	for(var i=0; i<clients.length; i++) {
		if(clients[i].ws.readyState == 1) clients[i].ws.send(msg.toString());
	}
});

// WebSocket received a message, transfer to ZMQ
var processClientMessage = function(ws, req, msg) {	
	if(msg.length !== 0)  {
		zmq_sock.send(msg);
	}
}

module.exports = {
	// WS /backend
	websocket: function(ws, req) {
		var userLoggedIn = null;
		ws.on('message', function(msg) {
			var sessID = req.sessionID;
			req.session.reload(function(err) {
				if(err) {
					userLoggedIn = false;
					removeClient(sessID);
				}else{
					if(req.session.loggedIn === true || req.session.loggedIn === false) {
						let cliIndex=findClient(req.sessionID);
						if(cliIndex === -1) {
							// Either a new client, or session ID changed
							if(sessID !== req.sessID) {
								// session id changed
								removeClient(sessID); // remove old one
								addClient(req.sessionID, ws); // add new one
							}else{
								// session id not changed
								// it is a new client
								addClient(req.sessionID, ws);
							}
						}else{
							// The client could be previously authenticated, but lost its websocket connection
							// Example: Refreshing the page
							if(clients[cliIndex].ws.readyState != 1) clients[cliIndex].ws=ws;
						}
						userLoggedIn = req.session.loggedIn;
					}else{
						userLoggedIn = false;
						removeClient(sessID); // logged out
					}
				}
				if(userLoggedIn === true) {
					processClientMessage(ws, req, msg);
				}else{
					ws.send("Unauthorized.");
					ws.close();
				}
			});
		});
	}, 
}