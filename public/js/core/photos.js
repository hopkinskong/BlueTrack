var photos = {
	viewRaw: function(mac) {
		btConsole.log("View Raw: " + mac);
		$('#view_photos_modal_static').modal({backdrop: 'static', keyboard: false});
		$("#view_photos_modal_static .modal-title").html("Viewing Raw Images");
		$("#view_photos_modal_static .modal-body").html('<p class="text-center"><strong>Loading, please wait...</strong></p>');
				
		callRPC("view_raw_images", {"mac": mac});
	}, 
	
	viewClassified: function(mac) {
		btConsole.log("View Classified: " + mac);
		$('#view_photos_modal_static').modal({backdrop: 'static', keyboard: false});
		$("#view_photos_modal_static .modal-title").html("Viewing Classified Images");
		$("#view_photos_modal_static .modal-body").html('<p class="text-center"><strong>Loading, please wait...</strong></p>');
				
		callRPC("view_classified_images", {"mac": mac});
	}, 
	
	displayImages: function(type, result) {
		btConsole.log("Showing Images");
		$("#view_photos_modal_static").modal('toggle');
		$("#view_photos_modal .modal-title").html(type + " Images");
		
		$("#view_photos_modal .modal-body").html('<p class="text-center">MAC Address: ' + result.mac + '</p>');
		
		if(result.photos.length > 2000) {
			$("#view_photos_modal .modal-body").html('<p class="text-center">Sorry, there are too many photos(' + result.photos.length + '), please view them locally.</p>');
		}else if(result.photos.length === 0 && type === "Raw") {
			$("#view_photos_modal .modal-body").html('<p class="text-center">Sorry, there are no photos captured.</p>');
		}else if(result.photos.length === 0 && type === "Classified") {
			$("#view_photos_modal .modal-body").html('<p class="text-center">Sorry, there are no photos being classified.</p>');
		}else{
			$("#view_photos_modal .modal-body").append('<table class="table table-striped text-center"></table>');
			var i=0;
			var currentRow=0;
			var background_colors = ["#BBBBBB", "#EEEEEE"];
			
			if(type === "Raw") {
				var imagesPerRow = 8;
				while(i<result.photos.length) {
					if(i%imagesPerRow == 0) {
						$("#view_photos_modal .modal-body table").append('<tr style="background-color: ' + background_colors[currentRow%background_colors.length] + '"></tr>');
						$("#view_photos_modal .modal-body table").append('<tr style="background-color: ' + background_colors[currentRow%background_colors.length] + '"></tr>');
						currentRow++;
					}
					$("#view_photos_modal .modal-body table tr:nth-last-child(2)").append('<td><b>' + result.photos[i].rssi + 'dBm</b></td>');
					if(result.photos[i].data != "") {
						$("#view_photos_modal .modal-body table tr:last").append('<td><img src="data:image/jpeg;base64,' + result.photos[i].data + '" class="img-responsive" /></td>');
					}else{
						$("#view_photos_modal .modal-body table tr:last").append('<td>No Image</td>');
					}
					i++;
				}
			}else if(type === "Classified") {
				var imagesPerRow = 4;
				while(i<result.photos.length) {
					if(i%imagesPerRow == 0) {
						$("#view_photos_modal .modal-body table").append('<tr style="background-color: ' + background_colors[currentRow%background_colors.length] + '"></tr>');
						currentRow++;
					}
					$("#view_photos_modal .modal-body table tr:last").append('<td><img src="data:image/jpeg;base64,' + result.photos[i].data + '" class="img-responsive classified-imgs" /></td>');
					i++;
				}
			}
		}
		$("#view_photos_modal").modal();
	},
	
	startNNM: function(mac) {
		btConsole.log("Starting NNM for MAC: " + mac);
		callRPC("start_nnm", {"mac": mac});
	}
};