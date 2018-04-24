var mainPage = {
	load: function(obj) {
		var page = $(obj).data("page");
		if(currentPage !== page) {
			// Load and render page
			$.ajax("/main/"+page)
			.done(function(pageHtml) {
				mainPage.render(pageHtml);
				currentPage = page;
			})
			.fail(function() {
				mainPage.render("Fail to load page.");
			});
			
			// Set all inactive
			$("#side-menu>li>a").removeClass("active");
			
			// Set active
			$(obj).addClass("active");
		}
	}, 
	render: function(html) {
		$("#page-wrapper").html(html);
		appUpdateUI();
	}
}