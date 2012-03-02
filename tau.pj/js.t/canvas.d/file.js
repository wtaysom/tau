/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

function log(message) {
	var text = document.createTextNode(
		message.toString()+"\n");
	$('#output').append(text);
}

var usejQueryAJAX = true;

if (usejQueryAJAX) {
	$.get("file.txt", handleResponse);
} else {
	sendRequest("file.txt", handleResponse);
}

function sendRequest(url, callback) {
	var req = new XMLHttpRequest();
	if (!req) return;
	req.open('GET', url, true);
	req.onreadystatechange = function () {
		if (req.readyState != 4) return;
		if (req.status != 200 && req.status != 304) {
			log("HTTP error "+req.status);
			return;
		}
		callback(req.responseText);
	}
	if (req.readyState == 4) return;
	req.send();
}

function handleResponse(response) {
	log("came back "+response.toString());
	eval("var v = ("+response+")");
	log(v.toString());
	var sum = 0;
	for (i in v) {
		log(v[i].toString());
		sum += v[i];
	}
	log("sum = "+sum.toString());
}
