var tracking = {
	enableTracking: function(mac) {
		btConsole.log("Enabling Tracking for MAC: " + mac);
		callRPC("enable_tracking", { "mac": mac });
	}, 
	
	disableTracking: function(mac) {
		btConsole.log("Disabling Tracking for MAC: " + mac);
		callRPC("disable_tracking", { "mac": mac });
	}, 
	
	rename: function(mac, current_name) {
		$("#tracking_rename_modal").modal();
		
		$("#tracking_rename_modal .modal-body").html('<div class="text-center">' + 
		'MAC Address: <strong>' + mac + '</strong><br />' + 
		'Current Name: <strong>' + current_name + '</strong><br /><br />' + 
		'New Name: <div class="form-inline"><input type="hidden" id="rename_mac" value="' + mac + '" /><input type="text" class="form-control" id="new_name" /></div>' + 
		'</div>');
		
		
	}, 
	
	rename_confirm: function() {
		$("#tracking_rename_modal").modal('toggle');
		
		var mac = $("#rename_mac").val();
		var new_name = $("#new_name").val();
		
		
		btConsole.log('Assigning victim "' + mac + '" a new name "' + new_name + '"');
		callRPC("rename_victim", {"mac": mac, "name": new_name});
	}
};