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

function drawGraph(g) {
	var leftMargin = 12;
	var rightMargin = 4;
	var topMargin = 3;
	var bottomMargin = 10
	var xmax = g.length - 1;
	var xmin = 0;
	var xtrans;
	var xscale;
	var ymax = max(g);;
	var ymin;
	var ytrans;
	var yscale;

	clear();
	function xlabel(text) {
		ctx.save();
		ctx.strokeStyle = 'rgb(0, 255, 0)';
		ctx.lineWidth = 0.5;
		ctx.strokeText(text, 50, 50);
		//ctx.fillText(text, 50, 50);
		ctx.restore();
	}
	xlabel(ymax.toString());
	ctx.strokeStyle = 'rgb(255, 0, 0)';
	ctx.lineWidth = 2;
	ctx.beginPath();
	ctx.moveTo(50, 50);
	ctx.lineTo(50, 250);
	ctx.lineTo(250, 250);
	ctx.moveTo(60, 60);
	ctx.lineTo(160, 160);
	ctx.stroke();
	/*
	*/
}

function genGraph(n) {
	var graph = new Array(n);
	for (var i = 0; i < n; i++) {
		graph[i] = Math.random() * 1000;
	}
	return graph;
}

function run() {
	canvas = $('#canvas').get(0);
	ctx = canvas.getContext('2d');
	
	$('body').click(function(e) {
		g = genGraph();
		drawGraph(g);
	});
	
	g = genGraph(10);
	drawGraph(g);
}

$(run);

})();
