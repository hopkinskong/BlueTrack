var appLoopIntervalHandle = null;
var backendWs = null;
var currentPage = null;
var allowedUpdate = true;

var context = {
	sniffer_connected: 0, 
	sniffers: [], 
	total_ble_devices: 0, 
	ble_devices: [], 
	victim_to_track_online: 0, 
	photos_captured: 0, 
	victims_photos_summary: [], 
	test_snapshots: [], 
	waiting_snapshot_id: -1, 
	tracking_list: [], 
	settings: {},
	btnnm_status: {}
};

var RPCFunctions = [];
function getRPCFunctionIndexByFunctionName(func) {
	var i, found=0;
	for(i=0; i<RPCFunctions.length; i++) {
		if(RPCFunctions[i].func_name === func) {
			found=1;
			break;
		}
	}
	if(found === 1) return i;
	return -1;
}
function getRPCFunctionIndexByRPCId(id) {
	var i, found=0;
	for(i=0; i<RPCFunctions.length; i++) {
		if(RPCFunctions[i].rpc_id === id) {
			found=1;
			break;
		}
	}
	if(found === 1) return i;
	return -1;
}
function callRPC(func, param) {
	if(getRPCFunctionIndexByFunctionName(func) !== -1) {
		// Still calling that function, don't call again
		return false;
	}else{
		var request = new JSON_RPC.Request(func, param);
		RPCFunctions.push({func_name: func, rpc_id: request.id});
		backendWs.send(request.toString());
		return true;
	}
}

function bluetrackWebApp() {
	// BlueTrack Console
	btConsole.init();
	btConsole.log("BlueTrack WebApp Starting up...");
	btConsole.log("Creating WebSocket to /backend");
	
	if(!("WebSocket" in window) || WebSocket === undefined) {
		btConsole.error("Error: WebSocket not supported.");
		terminateBluetrack();
		return;
	}
	
	
	// WebSocket
	backendWs = new WebSocket(((window.location.protocol === "https:") ? "wss://" : "ws://") + window.location.host + "/backend");
	
	backendWs.onclose = function(event) {
		if(event.code != 1000) {
			btConsole.error("WebSocket Error: " + util.getWebSocketErrorReason(event));
		}
		btConsole.log("WebSocket closed.");
		terminateBluetrack();
	}
	
	backendWs.onopen = function(event) {
		btConsole.log("WebSocket opened.");
		appLoopIntervalHandle = setInterval(appLoop, 500);
	}
	
	backendWs.onmessage = recvWebSocket;
	
	// Load Page
	mainPage.load($("#default-page"));
}

function terminateBluetrack() {
	if(appLoopIntervalHandle !== null) {
		clearInterval(appLoopIntervalHandle);
	}
	
	if(backendWs !== null) {
		backendWs.close();
	}
	
	btConsole.log("Application terminated.");
}

function processJsonRpcResponse(func_name, result) {
	//btConsole.log("Function: " + func_name + " Result: " + result);
	
	switch(func_name) {
	case "get_sniffers_count":
		context.sniffer_connected=result;
		if(context.waiting_snapshot_id != -1) {
			// check if waiting_snapshot_id is still connected
			// if not, release the user from un-cancelable dialog
			var found = 0;
			for(var i=0; i<context.sniffers.length; i++) {
				if(context.sniffers[i].id == context.waiting_snapshot_id) {
					found=1;
					break;
				}
			}
			// disconnected, release user
			if(found == 0) {
				btConsole.error("Waiting for snapshot, but sniffer disconnected");
				$("#snapshot_modal_static").modal('toggle');
				context.waiting_snapshot_id=-1;
			}
		}
		break;
	case "get_ble_devices_count":
		context.total_ble_devices=result;
		break;
	case "get_total_victims_to_track_count":
		context.victim_to_track_online=result;
		break;
	case "get_total_photos_captured_count":
		context.photos_captured=result;
		break;
	case "get_victims_photos_summary":
		context.victims_photos_summary=result;
		break;
	case "get_sniffers_list":
		context.sniffers=result;
		break;
	case "get_ble_devices_list":
		context.ble_devices=result;
		break;
	case "get_test_snapshots":
		if(result.length == 1) {
			$("#snapshot_modal_static").modal('toggle');
			context.waiting_snapshot_id=-1;
			
			$("#snapshot_modal .modal-title").html("Test Snapshot");
			var obj = result[0];
			
			//var raw = window.atob(obj.img);
			//console.log("Len: " + raw.length);
			//var array = new Uint8Array(new ArrayBuffer(raw.length));
			//for(var i=0; i<raw.length; i++) {
			//	array[i] = raw.charCodeAt(i);
			//}
			//var URL = window.URL.createObjectURL(new Blob([array], {'type': 'image/jpeg'}));
			//$("#snapshot_modal .modal-body").html('<p class="text-center">Sniffer #' + obj.id + '</p><img src="' + URL + '" class="img-responsive" />');
			$("#snapshot_modal .modal-body").html('<p class="text-center">Sniffer #' + obj.id + '</p><img src="data:image/jpeg;base64,' + obj.img + '" class="img-responsive" />');
			$("#snapshot_modal").modal();
		}
		break;
	case "get_tracking_list":
		context.tracking_list=result;
		break;
	case "get_settings":
		context.settings=result;
		break;
	case "view_raw_images":
		photos.displayImages("Raw", result);
		break;
	case "view_classified_images":
		photos.displayImages("Classified", result);
		break;
	case "get_btnnm_status":
		context.btnnm_status=result;
		break;
	}
	
	appUpdateUI();
}

function recvWebSocket(evt) {
	//btConsole.log("Recv=" + evt.data);
	if(evt.data) {
		var request = JSON_RPC.parse(evt.data);
		var i = getRPCFunctionIndexByRPCId(request.id);
		if(i !== -1) {
			if(request.error) {
				// Log the error
				btConsole.error("RPC Error: (" + RPCFunctions[i].func_name + "): " + request.error.message);
			}else{
				// Process the result
				processJsonRpcResponse(RPCFunctions[i].func_name, request.result);
			}
			// Remove RPCFunctions[i]
			RPCFunctions.splice(i, 1);
		}
	}
	
}

function appUpdateUI() {
	//btConsole.log("UpdateUI: " + currentPage);
	if(allowedUpdate === false) return;
	switch(currentPage) {
	case "dashboard":
		$("#dashboard_sniffer_count").html(context.sniffer_connected);
		$("#dashboard_ble_devices_count").html(context.total_ble_devices);
		$("#dashboard_victims_to_track_online_count").html(context.victim_to_track_online);
		$("#dashboard_photos_captured_count").html(context.photos_captured);
		break;
	case "sniffers":
		var i;
		$("#sniffers_tbody").html("");
		for(i=0; i<context.sniffers.length; i++) {
			var camera_flip = '<div class="btn-group" role="group">' + 
			'<button class="btn btn-info' + (context.sniffers[i].settings.h_flip == 1 ? ' active' : '') + '" onclick="sniffer_settings.hFlip(' + context.sniffers[i].id + ', ' + (context.sniffers[i].settings.h_flip == 1 ? 'false' : 'true') + ')">H-FLIP</button>' + 
			'<button class="btn btn-info' + (context.sniffers[i].settings.v_flip == 1 ? ' active' : '') + '" onclick="sniffer_settings.vFlip(' + context.sniffers[i].id + ', ' + (context.sniffers[i].settings.v_flip == 1 ? 'false' : 'true') + ')">V-FLIP</button>' + 
			'</div>';
			
			var camera_aec = '<div class="btn-group" role="group" style="margin-top:2px">';
			
			
			if(context.sniffers[i].settings.aec_on == 0) {
				camera_aec+='<button class="btn btn-primary" onclick="sniffer_settings.aec(' + context.sniffers[i].id + ', true, 0)">AEC</button>';
				camera_aec+='<button class="btn btn-primary active" onclick="sniffer_settings.manual_aec_show_modal(' + context.sniffers[i].id + ')">Manual EC: ' + context.sniffers[i].settings.aec_value + '</button>';
			}else{
				camera_aec+='<button class="btn btn-primary active" onclick="sniffer_settings.aec(' + context.sniffers[i].id + ', true, 0)">AEC</button>';
				camera_aec+='<button class="btn btn-primary" onclick="sniffer_settings.manual_aec_show_modal(' + context.sniffers[i].id + ')">Manual EC</button>';
			}
			
			camera_aec += '</div>';
			
			var led = '<div class="btn-group" role="group">' + 
			'<button class="btn btn-default' + (context.sniffers[i].settings.led == 2 ? ' active' : '') + '" onclick="sniffer_settings.led(' + context.sniffers[i].id + ', 2)">FLASH</button><button class="btn btn-default' + (context.sniffers[i].settings.led == 1 ? ' active' : '') + '" onclick="sniffer_settings.led(' + context.sniffers[i].id + ', 1)">FULL ON</button><button class="btn btn-default' + (context.sniffers[i].settings.led == 0 ? ' active' : '') + '" onclick="sniffer_settings.led(' + context.sniffers[i].id + ', 0)">OFF</button>' + 
			'</div>';
						
			var rssi_threshold = '<div>' + 
			'<button class="btn btn-success" onclick="sniffer_settings.rssi_threshold(' + context.sniffers[i].id + ', ' + context.sniffers[i].settings.rssi_threshold + ')">RSSI Threshold: ' + context.sniffers[i].settings.rssi_threshold + 'dBm</button>' + 
			'</div>';
			
			
			$("#sniffers_tbody").append("<tr>" + 
			"<td>" + context.sniffers[i].id + "</td>" + 
			"<td>" + context.sniffers[i].ip + "</td>" + 
			"<td>" + context.sniffers[i].bat + "%</td>" + 
			"<td>" + (context.sniffers[i].chg ? "Yes" : "No") + "</td>" + 
			"<td>" + context.sniffers[i].devs + "</td>" + 
			"<td>" + 
			'<table class="table"><tbody>' +
			'<tr><td><p class="label label-danger">Camera</p></td> <td class="text-center">' + camera_flip + '<br />' + camera_aec + '</td></tr>' + 
			'<tr><td><p class="label label-primary">LED</p></td> <td class="text-center">' + led + '</td></tr>' + 
			'<tr><td><p class="label label-warning">RSSI</p></td> <td class="text-center">' + rssi_threshold + '</td></tr>' + 
			'<tr><td><p class="label label-info">Actions</p></td> <td class="text-center"><button class="btn btn-warning" onclick="sniffer_settings.test_snapshot(' + context.sniffers[i].id + ')">Test Snapshot</button> <button class="btn btn-danger" onclick="sniffer_settings.reboot(' + context.sniffers[i].id + ')">Reboot</button></td></tr>' + 
			'</tbody></table>' +
			"</td></tr>");
			
		}
		$("#sniffers_sniffer_count").html(context.sniffer_connected);
		break;
	case "ble_devices":
		$("#ble_devices_tbody").html("");
		for(var i=0; i<context.ble_devices.length; i++) {
			var rssi_table='<table class="table"><tbody>';
			for(var j=0; j<context.ble_devices[i].rssis.length; j++) {
				rssi_table+="<tr><td>#" + context.ble_devices[i].rssis[j].id + ": </td><td>" + context.ble_devices[i].rssis[j].rssi + "</td></tr>";
			}
			rssi_table += "</tbody></table>";
			
			var track_settings = "";
			if(context.ble_devices[i].tracking === true) {
				track_settings = '<button class="btn btn-danger" onclick="tracking.disableTracking(\'' + context.ble_devices[i].mac + '\')">Disable Tracking</button>';
			}else{
				track_settings = '<button class="btn btn-success" onclick="tracking.enableTracking(\'' + context.ble_devices[i].mac + '\')">Enable Tracking</button>';
			}
			
			$("#ble_devices_tbody").append("<tr>" + 
			"<td>" + context.ble_devices[i].mac + "</td>" + 
			"<td>" + context.ble_devices[i].nodes + "</td>" + 
			"<td>" + rssi_table + "</td>" + 
			"<td>" + (context.ble_devices[i].tracking ? "Yes" : "No") + "</td>" + 
			"<td>" + track_settings + 
			"</td></tr>");
		}
		$("#ble_devices_ble_devices_count").html(context.total_ble_devices);
		break;
	case "tracking":
		$("#tracking_tbody").html("");
		
		for(var i=0; i<context.tracking_list.length; i++) {
			var track_settings='<button class="btn btn-warning" onclick="tracking.rename(\'' + context.tracking_list[i].mac + '\', \'' + util.escape_js_parm((context.tracking_list[i].name)) + '\')">Rename</button> ';
			
			if(context.tracking_list[i].active) {
				track_settings+='<button class="btn btn-danger" onclick="tracking.disableTracking(\'' + context.tracking_list[i].mac + '\')">Stop Tracking</button>';
			}else{
				track_settings+='<button class="btn btn-success" onclick="tracking.enableTracking(\'' + context.tracking_list[i].mac + '\')">Start Tracking</button>';
			}
			
			
			$("#tracking_tbody").append("<tr>" + 
			"<td>" + context.tracking_list[i].mac + "</td>" + 
			"<td>" + context.tracking_list[i].name + "</td>" + 
			"<td>" + (context.tracking_list[i].online ? "Yes" : "No") + "</td>" + 
			"<td>" + context.tracking_list[i].nodes + "</td>" + 
			"<td>" + (context.tracking_list[i].active ? "Yes" : "No") + "</td>" + 
			"<td>" + track_settings + 
			"</td></tr>");
		}
		$("#tracking_victims_to_track_online_count").html(context.victim_to_track_online);
		break;
	case "photos":
		$("#photos_tbody").html("");
		
		for(var i=0; i<context.victims_photos_summary.length; i++) {
			$("#photos_tbody").append('<tr>' +
			'<td>' + context.victims_photos_summary[i].name + '</td>' + 
			'<td>' + context.victims_photos_summary[i].mac + '</td>' + 
			'<td>' + context.victims_photos_summary[i].photos + '</td>' + 
			'<td>' + context.victims_photos_summary[i].nnm_processed + '/' + context.victims_photos_summary[i].photos + '</td>' + 
			'<td><button class="btn btn-success" onclick="photos.viewRaw(\'' + context.victims_photos_summary[i].mac + '\')">Raw</button> <button class="btn btn-warning" onclick="photos.viewClassified(\'' + context.victims_photos_summary[i].mac + '\')">Classified</button></td>' + 
			'<td><button class="btn btn-info" onclick="photos.startNNM(\'' + context.victims_photos_summary[i].mac + '\')">Start NNM</button></td>' + 
			'</tr>');
		}
		if(context.btnnm_status.status === "disconnected") {
			$("#photos_btnnm_status").html("<strong style=\"color:#DC143C;\">BTNNM Disconnected</strong>");
		}else if(context.btnnm_status.status === "idle") {
			$("#photos_btnnm_status").html("<strong style=\"color:#32CD32;\">BTNNM Connected, Idle</strong>");
		}else if(context.btnnm_status.status === "working") {
			$("#photos_btnnm_status").html("<strong style=\"color:#FF7F50;\">" + context.btnnm_status.mac + " Feature Extraction " + context.btnnm_status.progress + "/" + context.btnnm_status.max_images + "</strong>");
		}else{
			$("#photos_btnnm_status").html("<strong style=\"color:#DC143C;\">Unknown</strong>");
		}
		$("#photos_photos_captured_count").html(context.photos_captured);
		break;
	case "settings":
		$("#settings_rotate_cw_90deg").prop('checked', context.settings.rotate_cw_90deg);
	}
}

function appLoop() {
	backendWs.send(""); // Keep alive
	
	callRPC("get_sniffers_count", undefined);
	callRPC("get_ble_devices_count", undefined);
	callRPC("get_total_victims_to_track_count", undefined);
	callRPC("get_total_photos_captured_count", undefined);
	callRPC("get_sniffers_list", undefined);
	callRPC("get_ble_devices_list", undefined);
	callRPC("get_test_snapshots", undefined);
	callRPC("get_tracking_list", undefined);
	callRPC("get_victims_photos_summary", undefined);
	callRPC("get_settings", undefined);
	callRPC("get_btnnm_status", undefined);
	
	appUpdateUI();
}

$(function() {
	bluetrackWebApp();
	
	$("body").on("mousedown", function() {
		allowedUpdate=false;
	});

	$("body").on("mouseup", function() {
		allowedUpdate=true;
	});
});

