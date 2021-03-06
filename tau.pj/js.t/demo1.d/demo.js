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

var axisColor = '#08F';
var axisWidth = 1;
var gridWidth = axisWidth / 2;
var numXTicks = 6;
var numYTicks = 5;

function label(msg, x, y) {
	ctx.save();
	ctx.strokeStyle = '#FFF';
	ctx.lineWidth = 1;
	ctx.textAlign = 'right';
	ctx.strokeText(msg.toString(), x, y);
	ctx.restore();
}

function pr(msg) {	// For debug messages at top of screen
	ctx.save();
	ctx.strokeStyle = '#F00';
	ctx.clearRect(0, 0, canvas.width, 20);
	ctx.lineWidth = 1;
	ctx.strokeText(msg.toString(), 10, 10);
	ctx.restore();
}


function clear() {
	ctx.clearRect(0, 0, canvas.width, canvas.height);
}

/** Graph **/

function Graph(n) {
	this.values = [];
//	log(n);
	return graph;
}

Graph.prototype = {
	addRandomValue: function() {
		this.values.push(Math.random() * 1000);
	},

	max: function() {
		return Math.max.apply(null, this.values);
	},

	min: function() {
		return Math.min.apply(null, this.values);
	},

	draw: function() {
		var values = this.values;

		var leftMargin = 100;
		var rightMargin = 10;
		var topMargin = 20;
		var bottomMargin = 50;
		var xmax = values.length - 1;
		var ymax = this.max();
		var xmin = 0;
		var ymin = this.min();
		var xtrans;
		var xscale;
		var ytrans;
		var yscale;

//		log("values="+this.values.toString());

		plot();

		function plot() {
			clear();
			xAxis();
			yAxis();
			if (values.length == 0) return;
			ctx.save();
			ctx.strokeStyle = '#F00';
			ctx.lineWidth = 1;
			ctx.beginPath();
			ctx.moveTo(xScale(0), yScale(values[0]));
			for (i = 1; i < values.length; i++) {
				ctx.lineTo(xScale(i), yScale(values[i]));
			}
			ctx.stroke();
			ctx.restore();
		}

		function drawLine(xstart, ystart, xend, yend, color, width) {
			ctx.save();
			ctx.strokeStyle = color;
			ctx.lineWidth = width;
			ctx.beginPath();
			ctx.moveTo(xScale(xstart), yScale(ystart));
			ctx.lineTo(xScale(xend), yScale(yend));
			ctx.stroke();
			ctx.restore();
		}

		function xScale(x) {
			return Math.round(leftMargin +
				(x - xmin) / (xmax - xmin) *
				(canvas.width - leftMargin - rightMargin));
		}

		function yScale(y) {
			return Math.round(canvas.height - bottomMargin -
				(y - ymin) / (ymax - ymin) *
				(canvas.height - topMargin - bottomMargin));
		}

		function xAxis() {
			xLabel('x axis');
			drawLine(xmin, ymin, xmax, ymin, axisColor, axisWidth);
			label(Math.round(xmin), leftMargin, yScale(ymin));
			var dtick = (ymax - ymin) / numYTicks;
			var tick = dtick + ymin;
			for (var i = 0; i < numYTicks; i++) {
				label(Math.round(tick), leftMargin,
					yScale(tick));
				drawLine(xmin, tick, xmax, tick,
					'#0F0', gridWidth);
				tick += dtick;
			}
		}

		function yAxis() {
			yLabel('y axis');
			drawLine(xmin, ymin, xmin, ymax, axisColor, axisWidth);
			label(Math.round(tick), xScale(tick),
				canvas.height - bottomMargin + 10);
			var dtick = (xmax - xmin) / numXTicks;
			var tick = xmin + dtick;
			for (var i = 0; i < numXTicks; i++) {
				label(Math.round(tick), xScale(tick),
					canvas.height - bottomMargin + 10);
				drawLine(tick, ymin, tick, ymax,
					'#0F0', gridWidth);
				tick += dtick;
			}
		}

		function xLabel(text) {
			ctx.save();
			ctx.strokeStyle = '#0F0';
			ctx.lineWidth = 1;
			ctx.strokeText(text, canvas.width / 2,
				canvas.height - bottomMargin / 2);
			ctx.restore();
		}

		function yLabel(text) {
			ctx.save();
			ctx.strokeStyle = '#0F0';
			ctx.lineWidth = 1;
			ctx.translate(leftMargin / 2, canvas.height / 2);
			ctx.rotate(-Math.PI/2);
			ctx.strokeText(text, 0, 0);
			ctx.restore();
		}
	}
}

/** Run **/

var graph = new Graph(1);

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
/*
		delete graph;
		graph = new Graph(2);
*/
		graph.values = [];
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

var oldv;

function handleResponse(response) {
	eval("var v = " + response);
	graph.values.push(v);
	graph.draw();
//	log(v);
}

})();
