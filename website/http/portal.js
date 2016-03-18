var suppressed = 0;
var magic_clicks = 0;
var title_views = 0;
var portal_color_saved = 0;

function portal_guy_click(){
	portalflipdiv.classList.toggle("flip")
	magic_clicks++;
	if (magic_clicks == 5){
		titletext.innerHTML = "Unlocked!"
		$(".menu li:eq(4)").show();
		$(".menu li:eq(5)").show();
		
		suppressed = 1;
		$('#menuicon').trigger('click');
	}
}

function force_menu(){
	suppressed = 1;
	$('#menuicon').trigger('click');
}
function fadeit(){
	$( "#loadingtxt" ).fadeOut("slow");
}

$(document).ready(function() {
	$('.menu').dropit();	
	$(".menu ul.dropit-submenu a").addClass("blue");
	$(".menu li:eq(4)").hide();
	$(".menu li:eq(5)").hide();
});
