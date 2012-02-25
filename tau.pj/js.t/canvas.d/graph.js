(function() {

/** Simple Graphs **/

var canvas;
var ctx;

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
		return this.values.reduce(function(m, v) {
			return Math.max(m, v);
		}, 0);
	},
	
	min: function() {
		return this.values.reduce(function(m, v) {
			return Math.min(m, v);
		}, 0);
	},
	
	draw: function() {
		var values = this.values;
		
		var leftMargin = 40;
		var rightMargin = 10;
		var topMargin = 10;
		var bottomMargin = 40;
		var mx = new Point(values.length, this.max());
		var mn = new Point(0, this.min());
		var xtrans;
		var xscale;
		var ytrans;
		var yscale;
		
		plot();
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
		
		function plot() {
			if (values.length == 0) return;
			xaxis();
			ctx.save();
			ctx.strokeStyle = '#F00';
			ctx.lineWidth = 1;
			ctx.beginPath();
			ctx.moveTo(xScale(0), yScale(values[0]));
			for (i = 1; i < values.length; i++) {
				ctx.lineTo(xScale(i), yScale(values[i]));
			}
			ctx.stroke();
			ctx.closePath();
			ctx.restore();
			/*
			*/
		}
		
		function xScale(x) {
			return leftMargin +
				(x - mn.x) / (mx.x - mn.x) * 
				(canvas.width - leftMargin - rightMargin);
		}
		
		function yScale(y) {
			return canvas.height - bottomMargin -
				(y - mn.y) / (mx.y - mn.y) *
				(canvas.height - topMargin - bottomMargin);
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
