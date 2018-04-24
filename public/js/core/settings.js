var settings = {
	rotate_cw_90deg: function(val) {
		btConsole.log("Rotate 90° CW enabled=" + val);
		callRPC("rorate_cw_90Deg", {"enabled": val});
	}
};