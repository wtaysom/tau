(function() {

/** Implementation of Per Bak's Sand Piles **/

var canvas;
var ctx;
var color = [	'rgb(255, 0, 0)', 'rgb(0, 255, 0)', 'rgb(0, 0, 255)',
		'rgb(255, 255, 0)', 'rgb(0, 255, 255)', 'rgb(255, 0, 255)',
		'rgb(255, 255, 255)', 'rgb(0, 0, 0)'];
var stop = false;
var size = 10;

function sandpile(x, y) {
	var a = new Array(x);
	for (var i = 0; i < x; i++) {
		a[i] = new Array(y);
		for (var j = 0; j < y; j++) {
			a[i][j] = 0;
		}
	}
	return a;
}

function inc(a, x, y) {
	if (x < 0 || x >= a.length) return;
	if (y < 0 || y >= a[x].length) return;
	if (++a[x][y] >= 4) {
		a[x][y] -= 4;
		inc(a, x + 1, y);
		inc(a, x - 1, y);
		inc(a, x, y + 1);
		inc(a, x, y - 1);
	}
	ctx.fillStyle = color[a[x][y]];
	ctx.fillRect(x * size, y * size, size, size);
}

function dribble(a) {
	var x = Math.floor(Math.random() * a.length);
	var y = Math.floor(Math.random() * a[x].length);
	inc(a, x, y);
}

function drawSand(a) {
	for (var x = 0; x < a.length; x++) {
		for (var y = 0; y < a[x].length; y++) {
			ctx.fillStyle = color[a[x][y]]; // 'rgb('+r+', 0, '+b+')';
			ctx.fillRect(x * size, y * size, size, size);
		}
	}
}

function runTests() {
	canvas = $('#myCanvas').get(0);
	ctx = canvas.getContext('2d');
	$(canvas).click(function(e) {
		stop = true;
	});
	
	ctx.clearRect(0, 0, canvas.width, canvas.height);
	var xmax = Math.floor(canvas.height / size);
	var ymax = Math.floor(canvas.width / size);
	var a = sandpile(xmax, ymax);
	$('body').css('backgroundColor', 'rgb(255, 255, 255)');
	step();
	function step() {
		if (stop) return;
		dribble(a);
//		drawSand(a);
		setTimeout(step, 1);
	};
		
}

$(runTests);

})();
