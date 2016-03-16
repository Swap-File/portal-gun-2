function increment_idle_view(){
	views++;
	if (views > 1 || views < 0) { views = 0;}
	switch(views) {
	case 0:
		leftlogo.src = "assets/lo.png"
		rightlogo.src = "assets/rb.png"
		break;
	case 1:
		leftlogo.src = "assets/lb.png"
		rightlogo.src = "assets/ro.png"
		break;
	}
	setCookie("views", views, 365);
}

function swapper(state,name){
	theicon = document.getElementById(name + "icon");
	thediv = document.getElementById(name + "div");
	if (state == 1){
		if (button_hovers % 2){
			theicon.style.color = '#F79126';
		}else{
			theicon.style.color = '#16ABE3';
		}
		thediv.style.backgroundColor = "#707070";
		button_hovers++;
	}else{
		theicon.style.color = '#000000';
		thediv.style.backgroundColor = "#404040";
	}
}

function setCookie(cname, cvalue, exdays) {
	var d = new Date();
	d.setTime(d.getTime() + (exdays*24*60*60*1000));
	var expires = "expires="+d.toUTCString();
	document.cookie = cname + "=" + cvalue + "; " + expires;
}

function getCookie(cname) {
	var name = cname + "=";
	var ca = document.cookie.split(';');
	for(var i=0; i<ca.length; i++) {
		var c = ca[i];
		while (c.charAt(0)==' ') c = c.substring(1);
		if (c.indexOf(name) == 0) return c.substring(name.length, c.length);
	}
	return "";
}