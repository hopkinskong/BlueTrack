var sniffer_settings = {
	vFlip: function(id, val) {
		btConsole.log("vFlip for #" + id + " enabled=" + val);
		callRPC("vFlip", {"enabled": val, "sniffer_id": id});
	}, 
	hFlip: function(id, val) {
		btConsole.log("hFlip for #" + id + " enabled=" + val);
		callRPC("hFlip", {"enabled": val, "sniffer_id": id});
	}, 
	reboot: function(id) {
		btConsole.log("Rebooting #" + id);
		callRPC("reboot", {"sniffer_id": id});
	}, 
	test_snapshot: function(id) {
		btConsole.log("Testing Snapshot #" + id);
		$('#snapshot_modal_static').modal({backdrop: 'static', keyboard: false});
		$("#snapshot_modal_static .modal-title").html("Test Snapshot");
		$("#snapshot_modal_static .modal-body").html('<p class="text-center"><strong>Loading, please wait...</strong></p>');
		
		context.waiting_snapshot_id=id;
		
		callRPC("test_snapshot", {"sniffer_id": id});
	}, 
	led: function(id, mode) {
		btConsole.log("LED for #" + id + " mode=" + mode);
		callRPC("LED", {"sniffer_id": id, "mode": mode });
	},
	rssi_threshold: function(id, current_setting) {
		$("#sniffer_rssi_config_modal").modal();
		
		$("#sniffer_rssi_config_modal .modal-body").html('<div class="text-center">' + 
		'Sniffer ID: <strong>#' + id + '</strong><br />' + 
		'Current Configured RSSI Threshold: <strong>' + current_setting + 'dBm</strong><br /><br />' + 
		'New RSSI Threshold Value(dBm): <div class="form-inline"><input type="hidden" id="update_rssi_threshold_id" value="' + id + '" /><input type="text" class="form-control" id="update_rssi_threshold_dbm" /></div>' + 
		'</div>');
	},
	rssi_threshold_confirm: function() {
		$("#sniffer_rssi_config_modal").modal('toggle');
		
		var id = parseInt($("#update_rssi_threshold_id").val());
		var dbm = parseInt($("#update_rssi_threshold_dbm").val());
		
		btConsole.log('Updating RSSI Threshold for #' + id + ' for RSSI=' + dbm + 'dBm');
		callRPC("rssi_threshold", {"sniffer_id": id, "threshold": dbm});
	},
	aec: function(id, aec_on, aec_value) {
		if(aec_on === true) {
			btConsole.log('Set AEC for #' + id + ' ON=' + aec_on);
		}else{
			btConsole.log('Set AEC for #' + id + ' ON=' + aec_on + ' VAL=' + aec_value);
		}
		callRPC("AEC", {"sniffer_id": id, "aec_on": aec_on, "aec_value": aec_value});
	}, 
	manual_aec_show_modal: function(id) {
		$("#aec_config_modal").modal();
		$("#aec_config_modal .modal-body").html('<div class="text-center">' + 
		'Sniffer ID: <strong>#' + id + '</strong><br />' + 
		'New AEC Value (0~65535): <div class="form-inline"><input type="hidden" id="manual_aec_id" value="' + id + '" /><input type="text" class="form-control" id="manual_aec_value" /></div>' + 
		'</div>');
	},
	manual_aec_confirm: function() {
		$("#aec_config_modal").modal('toggle');
		
		var id = parseInt($("#manual_aec_id").val());
		var aec_val = parseInt($("#manual_aec_value").val());
		
		this.aec(id, false, aec_val);
	}
};