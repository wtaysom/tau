/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */
function sendRequest(url,callback) {
	var req = new XMLHttpRequest();
	if (!req) return;
	req.open("GET", url, true);
	req.onreadystatechange = function () {
		if (req.readyState != 4) return;
		if (req.status != 200 && req.status != 304) {
			alert('HTTP error ' + req.status);
			return;
		}
		callback(req);
	}
	if (req.readyState == 4) return;
	req.send();
}

sendRequest('file.txt',handleRequest);

function handleRequest(req) {
	alert('came back ' + req.responseText.toString());
	eval("var v = ("+req.responseText+")");
	alert(v.toString());
	var sum = 0;
	for (i in v) {
		alert(i.toString());
		sum += i;
	}
	alert('sum = ' + sum.toString());
}

var ajaxRequest = new XMLHttpRequest();

// Create a function that will receive data sent from the server
ajaxRequest.onreadystatechange = function(){
	if(ajaxRequest.readyState == 4){
		var cType = this.getResponseHeader('Content-Type');
		alert(cType);
		alert(ajaxRequest.responseText);
	}
}

//ajaxRequest.open('GET', 'file.txt', true);
//ajaxRequest.send();
