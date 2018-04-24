var util = {
	paddy: function (n, p, c) {
		var pad_char = typeof c !== 'undefined' ? c : '0';
		var pad = new Array(1 + p).join(pad_char);
		return (pad + n).slice(-pad.length);
	}, 
	getTimeString: function() {
		var d = new Date();
		return (util.paddy(d.getHours(), 2) + ":" + util.paddy(d.getMinutes(), 2) + ":" + util.paddy(d.getSeconds(), 2) + "." + util.paddy(d.getMilliseconds(), 3));
	}, 
	getWebSocketErrorReason: function(event) {
		if (event.code == 1000) return "Normal closure, meaning that the purpose for which the connection was established has been fulfilled.";
		else if(event.code == 1001) return "An endpoint is \"going away\", such as a server going down or a browser having navigated away from a page.";
		else if(event.code == 1002) return "An endpoint is terminating the connection due to a protocol error";
		else if(event.code == 1003) return "An endpoint is terminating the connection because it has received a type of data it cannot accept (e.g., an endpoint that understands only text data MAY send this if it receives a binary message).";
		else if(event.code == 1004) return "Reserved. The specific meaning might be defined in the future.";
		else if(event.code == 1005) return "No status code was actually present.";
		else if(event.code == 1006) return "The connection was closed abnormally, e.g., without sending or receiving a Close control frame";
		else if(event.code == 1007) return "An endpoint is terminating the connection because it has received data within a message that was not consistent with the type of the message (e.g., non-UTF-8 [http://tools.ietf.org/html/rfc3629] data within a text message).";
		else if(event.code == 1008) return "An endpoint is terminating the connection because it has received a message that \"violates its policy\". This reason is given either if there is no other sutible reason, or if there is a need to hide specific details about the policy.";
		else if(event.code == 1009) return "An endpoint is terminating the connection because it has received a message that is too big for it to process.";
		else if(event.code == 1010) return "An endpoint (client) is terminating the connection because it has expected the server to negotiate one or more extension, but the server didn't return them in the response message of the WebSocket handshake. <br /> Specifically, the extensions that are needed are: " + event.reason;
		else if(event.code == 1011) return "A server is terminating the connection because it encountered an unexpected condition that prevented it from fulfilling the request.";
		else if(event.code == 1015) return "The connection was closed due to a failure to perform a TLS handshake (e.g., the server certificate can't be verified).";
		else return "Unknown reason";
	}, 
	escape_sql: function(unsafe) {
		return unsafe.replace(/'/g, "''");
	},
	escape_js_parm: function(unsafe) {
		return unsafe.replace(/&/, "&amp;").replace(/"/g, "&quot;").replace(/'/g, "\\'");
	}
};