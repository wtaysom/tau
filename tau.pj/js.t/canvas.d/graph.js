(function() {

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

/** Point **/

function Point(x, y) {
	this.x = x;
	this.y = y;
}

Point.prototype = {
	add: function(p) {
		return new Point(this.x + p.x, this.y + p.y);
	}
}

/** Graph **/

function Graph(n) {
	this.values = [];
	for (var i = 0; i < n; i++) {
		this.addRandomValue();
	}
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
		
		var leftMargin = 60;
		var rightMargin = 10;
		var topMargin = 20;
		var bottomMargin = 50;
		var mx = new Point(values.length - 1, this.max());
		var mn = new Point(0, this.min());
		var xtrans;
		var xscale;
		var ytrans;
		var yscale;
		
		plot();
		
		function plot() {
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
			/*
			*/
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
				(x - mn.x) / (mx.x - mn.x) * 
				(canvas.width - leftMargin - rightMargin));
		}
		
		function yScale(y) {
			return Math.round(canvas.height - bottomMargin -
				(y - mn.y) / (mx.y - mn.y) *
				(canvas.height - topMargin - bottomMargin));
		}
		
		function xAxis() {
			xLabel('x axis');
			drawLine(mn.x, mn.y, mx.x, mn.y, axisColor, axisWidth);
			label(Math.round(mn.x), leftMargin, yScale(mn.y));
			var dtick = (mx.y - mn.y) / numYTicks;
			var tick = dtick + mn.y;
			for (var i = 0; i < numYTicks; i++) {
				label(Math.round(tick), leftMargin, yScale(tick));
				drawLine(mn.x, tick, mx.x, tick,
					'#0F0', gridWidth);
				tick += dtick;
			}
		}
		
		function yAxis() {
			yLabel('y axis');
			drawLine(mn.x, mn.y, mn.x, mx.y, axisColor, axisWidth);
			label(Math.round(tick), xScale(tick), canvas.height - bottomMargin + 10);
			var dtick = (mx.x - mn.x) / numXTicks;
			var tick = mn.x + dtick;
			for (var i = 0; i < numXTicks; i++) {
				label(Math.round(tick), xScale(tick), canvas.height - bottomMargin + 10);
				drawLine(tick, mn.y, tick, mx.y,
					'#0F0', gridWidth);
				tick += dtick;
			}
		}
	
		function xLabel(text) {
			ctx.save();
			ctx.strokeStyle = '#0F0';
			ctx.lineWidth = 1;
			ctx.strokeText(text, canvas.width / 2, canvas.height - bottomMargin / 2);
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

function step() {
	clear();
	graph.addRandomValue();
	graph.draw();
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
	
	resume();
}

$(run);

})();
