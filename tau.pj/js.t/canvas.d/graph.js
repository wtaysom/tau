(function() {

/** Simple Graphs **/

var canvas;
var ctx;

function clear() {
	ctx.clearRect(0, 0, canvas.width, canvas.height);
}

function max(g) {
	if (g.length == 0) return 0;
	var m = g[0];
	for (var i = 1; i < g.length; i++) {
		if (m < g[i]) m = g[i];
	}
	return m;
}

function min(g) {
	if (g.length == 0) return 0;
	var m = g[0];
	for (var i = 1; i < g.length; i++) {
		if (g[i] < m) m = g[i];
	}
	return m;
}

function point(x, y) {
	return {x: x, y: y};
}

function addPoints(a, b) {
	return point(a.x + b.x, a.y + b.y);
}

function drawGraph(g) {
	var leftMargin = 40;
	var rightMargin = 10;
	var topMargin = 10;
	var bottomMargin = 40;
	var mx = point(g.length, max(g));
	var mn = point(0, min(g));
	var xtrans;
	var xscale;
	var ytrans;
	var yscale;

	clear();
	function xlabel(text) {
		ctx.save();
		ctx.strokeStyle = '#0F0';
		ctx.lineWidth = 1;
		ctx.strokeText(text, 150, 150);
		ctx.restore();
	}
	function ylabel(text) {
		ctx.save();
		ctx.strokeStyle = '#0F0';
		ctx.lineWidth = 1;
		ctx.translate(20, 150);
		ctx.rotate(Math.PI/2);
		ctx.strokeText(text, 0, 0);
		ctx.restore();
	}
	function xScale(x) {
		return (x - mn.x) / (mx.x - mn.x) * (canvas.width - leftMargin - rightMargin) + leftMargin;
	}
	function yScale(y) {
		return canvas.height - bottomMargin - (y - mn.y) / (mx.y - mn.y) * (canvas.height - topMargin - bottomMargin);
	}
	function xaxis() {
		ctx.save();
		ctx.strokeStyle = '#00F';
		ctx.lineWidth = 1;
		ctx.beginPath();
		ctx.moveTo(xScale(0), yScale(0));
		ctx.lineTo(xScale(mx.x), yScale(0));
		ctx.closePath();
		ctx.stroke();
		ctx.restore();
	}
	function plot(g) {
		if (g.length == 0) return;
		xaxis();
		ctx.save();
		ctx.strokeStyle = '#F00';
		ctx.lineWidth = 1;
		ctx.beginPath();
		ctx.moveTo(xScale(0), yScale(g[0]));
		for (i = 1; i < g.length; i++) {
			ctx.lineTo(xScale(i), yScale(g[i]));
		}
		ctx.stroke();
		ctx.closePath();
		ctx.restore();
		/*
		*/
	}
	plot(g);
	/*
	xlabel(mx.y.toString());
	ctx.strokeStyle = 'rgb(255, 0, 0)';
	ctx.lineWidth = 1;
	ctx.beginPath();
	ctx.moveTo(50, 50);
	ctx.lineTo(50, 250);
	ctx.lineTo(250, 250);
	ctx.moveTo(60, 60);
	ctx.lineTo(160, 160);
	ctx.stroke();
	*/
}

function genGraph(n) {
	var graph = new Array(n);
	for (var i = 0; i < n; i++) {
		graph[i] = Math.random() * 1000;
	}
	return graph;
}

var interval = 1000;
var paused = false;
var intervalId;

function pause() {
	paused = true;
	clearInterval(intervalId);
}

function resume() {
	paused = false
	clearInterval(intervalId);
	intervalId = setInterval(function() {
		g.push(Math.random() * 1000);
		drawGraph(g);
	}, interval)
}

function run() {
	canvas = $('#canvas').get(0);
	ctx = canvas.getContext('2d');
	
	$('body').click(function(e) {
		if (paused) {
			resume();
		} else {
			pause();
		}
	});
	
/*
	$('body').click(function(e) {
		g.push(Math.random() * 1000); // g = genGraph(100);
		drawGraph(g);
	});
*/

	g = genGraph(1);
	resume();
}

$(run);

})();
