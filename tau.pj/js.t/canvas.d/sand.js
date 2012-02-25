(function() {

/** Implementation of Per Bak's Sand Piles **/

var random = true;
var grainSize = 5;
var grainsPerInterval = 10;	// 10,000 still looks interesting by 100,000 doesn't
var interval = 1;
var color = [
		'#320', '#640', '#960',
		'#B80', '#F00', '#F00',
		'#F00', '#F00', '#F00'];

var canvas;
var ctx;

var paused = false;
var intervalId;

var sandPile;
var overflowQueue = [];

function initSandPile(width, height) {
	sandPile = new Array(width);
	for (var i = 0; i < width; i++) {
		sandPile[i] = new Array(height);
		for (var j = 0; j < height; j++) {
			sandPile[i][j] = 0;
		}
	}
}

function drawGrain(x, y) {
	ctx.fillStyle = color[sandPile[x][y]];
	ctx.fillRect(x * grainSize, y * grainSize, grainSize, grainSize);
}

function overflow() {
	overflowQueue.push(arguments);
}

function inc(x, y) {	
	if (x < 0 || x >= sandPile.length) return;
	if (y < 0 || y >= sandPile[x].length) return;
	if (++sandPile[x][y] >= 4) {
		sandPile[x][y] -= 4;
		overflow(x + 1, y);
		overflow(x - 1, y);
		overflow(x, y + 1);
		overflow(x, y - 1);
	}
	drawGrain(x, y);
}

function dribble() {
	var x = Math.floor(random ? Math.random() * sandPile.length
	                          : sandPile.length / 2);
	var y = Math.floor(random ? Math.random() * sandPile[x].length
	                          : sandPile[x].length / 2);
	inc(x, y);
}

function pause() {
	paused = true;
	clearInterval(intervalId);
}

function resume() {
	paused = false
	clearInterval(intervalId);
	intervalId = setInterval(function() {
		for (var i = 0; i < grainsPerInterval; ++i) {
			var overflow = overflowQueue.shift();
			if (overflow) {
				inc.apply(this, overflow)
			} else {
				dribble();
			}
		}
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
	
	var pileWidth = Math.floor(canvas.width / grainSize);
	var pileHeight = Math.floor(canvas.height / grainSize);
	initSandPile(pileWidth, pileHeight);
	
	resume();
}

$(run);

})();
