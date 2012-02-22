(function() {

/** Test Running **/

var tests = [];

function defTest(fn) {
	tests.push(fn);
}

var canvas;
var ctx;

function runTests(n) {
	canvas = $('#myCanvas').get(0);
	ctx = canvas.getContext('2d');
	
	var count = 0;
	step();
	
	function step() {		
		var test = tests[count++];
		if (test) {
			if (false) {
				ctx.clearRect(0, 0, canvas.width, canvas.height);
			}
			
			test();
			setTimeout(step, 500);
		}
	};
}

$(runTests);

/** Tests **/

defTest(function() { // Rectangle
	ctx.fillRect(50, 50, 100, 100);
	ctx.strokeRect(50, 50, 100, 100);
});

defTest(function() { // Triangle
	ctx.strokeStyle = 'rgb(255, 0, 0)';
	ctx.fillStyle = 'rgb(0, 255, 0)';
	ctx.lineWidth = 20;
	ctx.beginPath();
	ctx.moveTo(50, 50);
	ctx.lineTo(50, 250);
	ctx.lineTo(250, 250);
	ctx.closePath();
	if (false) {
		ctx.stroke();
	} else {
		ctx.fill();
	}
});

defTest(function() { // Color
	ctx.fillStyle = 'rgb(255, 0, 0)';
	ctx.fillRect(50, 50, 100, 100);
});


defTest(function() { // Fat lines
	ctx.lineWidth = 20;
	ctx.strokeStyle = 'rgb(255, 0, 0)';
	ctx.strokeRect(50, 50, 100, 100);
});

defTest(function() { // Using clear to clear part of the drawing
	ctx.fillRect(50, 50, 100, 100);
	ctx.fillStyle = 'rgb(255, 0, 0)';
	ctx.fillRect(200, 50, 100, 100);
	if (true) ctx.clearRect(50, 50, 100, 100);
});

defTest(function() { // Circles
	ctx.beginPath();
	ctx.arc(100, 100, 50, 0, Math.PI * 2, false);
	ctx.closePath();
	ctx.stroke();
});

defTest(function() { // Bezier Paths
	ctx.lineWidth = 8;
	ctx.beginPath();
	ctx.moveTo(50, 150);
	ctx.quadraticCurveTo(250, 50, 450, 150);
	ctx.stroke();
});

defTest(function() { // Quadratic Bezier Paths
	ctx.lineWidth = 8;
	ctx.beginPath();
	ctx.moveTo(50, 150);
	ctx.bezierCurveTo(150, 50, 350, 250, 450, 150);
	ctx.stroke();
});

defTest(function() { // Drawing state
	ctx.fillStyle = 'rgb(0, 0, 255)';
	ctx.save();
	ctx.fillRect(50, 50, 100, 100);
	ctx.fillStyle = 'rgb(255, 0, 0)';
	ctx.fillRect(200, 50, 100, 100);
	ctx.restore();
	ctx.fillRect(350, 50, 100, 100);
});

defTest(function() { // Translation
	ctx.fillRect(0, 0, 100, 100);
	ctx.save();
	ctx.translate(100, 100);
	ctx.fillStyle = 'rgb(0, 0, 255)';
	ctx.fillRect(0, 0, 100, 100);
	ctx.restore();
});

defTest(function() { // Scaling - multiplies everything by scalling factors
	ctx.save();
	ctx.translate(100, 100);
	ctx.scale(2, 2);
	ctx.fillRect(0, 0, 100, 100);
	ctx.restore();
});

defTest(function() { // Rotation - rotates around origin
	ctx.save();
	ctx.translate(150, 150); // Translate to centre of square
	ctx.rotate(Math.PI/4); // Rotate 45 degrees (in radians)
	ctx.fillRect(-50, -50, 100, 100); // Centre at the rotation point
	ctx.restore();
});

defTest(function() { // Shadows
	ctx.save();
	ctx.shadowBlur = 15;
	ctx.shadowColor = 'rgb(0, 0, 0)';
	ctx.fillRect(100, 100, 100, 100);
	ctx.restore();
	ctx.save();
	ctx.shadowBlur = 0;
	ctx.shadowOffsetX = 6;
	ctx.shadowOffsetY = 6;
	ctx.shadowColor = 'rgba(125, 125, 125, 0.5)'; // Transparent grey
	ctx.fillRect(300, 100, 100, 100);
	ctx.restore();
});

defTest(function() { // Gradients
	var gradient = ctx.createLinearGradient(0, 0, 0, canvas.height);
	gradient.addColorStop(0, 'rgb(255, 255, 255)');
	gradient.addColorStop(1, 'rgb(0, 0, 0)');
	
	ctx.save();
	ctx.fillStyle = gradient;
	ctx.fillRect(0, 0, canvas.width, canvas.height);
	ctx.restore();
});

defTest(function() { // Radial Gradients
	var gradient = ctx.createRadialGradient(350, 350, 0, 50, 50, 100);
	gradient.addColorStop(0, 'rgb(0, 0, 0)');
	gradient.addColorStop(1, 'rgb(125, 125, 125)');
	
	ctx.save();
	ctx.fillStyle = gradient;
	ctx.fillRect(0, 0, canvas.width, canvas.height);
	ctx.restore();
});

defTest(function() { // Radial Gradients
	var canvasCentreX = canvas.width/2;
	var canvasCentreY = canvas.height/2;

	var gradient = ctx.createRadialGradient(
		canvasCentreX, canvasCentreY, 250,
		canvasCentreX, canvasCentreY, 0);
	gradient.addColorStop(0, 'rgb(0, 0, 0)');
	gradient.addColorStop(1, 'rgb(125, 125, 125)');

	ctx.save();
	ctx.fillStyle = gradient;
	ctx.fillRect(0, 0, canvas.width, canvas.height);
	ctx.restore();
});

defTest(function() { // Images
	ctx.clearRect(0, 0, canvas.width, canvas.height);
	var image = new Image();
	image.src = 'sample.jpg';
	$(image).load(function() {
		ctx.drawImage(image, 0, 0, 300, 400);
	});
	$('body').css('backgroundColor', 'rgb(255, 0, 0)');
	$(canvas).click(function(e) {
		var canvasOffset = $(canvas).offset();
		var canvasX = Math.floor(e.pageX-canvasOffset.left);
		var canvasY = Math.floor(e.pageY-canvasOffset.top);
		
		/* To avoid SECURITY_ERR, run through:
		
			canvas.d> python -m SimpleHTTPServer

		pointing your browser to:

			http://127.0.0.1:8000/2d.html
		
		*/
		var imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
		var pixels = imageData.data;
		var pixelRedIndex = ((canvasY - 1) * (imageData.width * 4))
			+ ((canvasX - 1) * 4);
		var r = pixels[pixelRedIndex];
		var g = pixels[pixelRedIndex+1];
		var b = pixels[pixelRedIndex+2];
		var a = pixels[pixelRedIndex+3];
		var pixelcolor = 'rgba('+r+','+g+','+b+','+a+')';
		$('body').css('backgroundColor', pixelcolor);
	});
});

})();
