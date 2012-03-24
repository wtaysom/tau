/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Distributed under the terms of the GNU General Public License v2
 */

(function() {

function log(message) {
	var text = document.createTextNode(
		message.toString()+"\n");
	$('#log').append(text);
}

/** Simple Graphs **/

var canvas;
var ctx;

var interval = 1000;
var paused = false;
var intervalId;
var plots = ['rand', 'fib', 'sine', 'proc'];
var thePlot = 'sine';

function pickAplot() {
	$("#sidebar").append("<h2>Pick:</h2> <ul>");
	
	for (i in plots) {
		$("#sidebar").append("<li><a href='#'>" + plots[i] + "</a></li>");
	}
	$("#sidebar").append("</ul>");
	
	$("#sidebar a").click(function(e) {
		$("#sidebar").append("<p>Clicked " + $(this).html() + "</p>");
		thePlot = $(this).html();
//		log(thePlot);
	});
}

function step() {
	$.get(thePlot, handleResponse);
}

function pause() {
	paused = true;
	clearInterval(intervalId);
}

function resume() {
	paused = false
	clearInterval(intervalId);
	intervalId = setInterval(step, interval)
}

function run() {
	canvas = $('#canvas').get(0);
	ctx = canvas.getContext('2d');

	$('#canvas').click(function(e) {
		if (paused) {
			resume();
		} else {
			pause();
		}
	});
	
	pickAplot();

	resume();
}

$(run);

var smoothie = new SmoothieChart({
    grid: { strokeStyle:'rgb(125, 0, 0)', fillStyle:'rgb(60, 0, 0)',
           lineWidth: 1, millisPerLine: 250, verticalSections: 6, },
    labels: { fillStyle:'rgb(60, 0, 0)' }
});
smoothie.streamTo(document.getElementById("canvas"), 1000 /* delay */);
// Data
var line1 = new TimeSeries();

smoothie.addTimeSeries(line1,
    { strokeStyle:'rgb(0, 255, 0)', fillStyle:'rgba(0, 255, 0, 0.4)', lineWidth:3 });

function handleResponse(response) {
	eval("var v = " + response);
	line1.append(new Date().getTime(), v);
	log(v);
}

})();
