/*
	Just: Some Simple JavaScript Inheritance
	By William Taysom
	2011
*/
(function() {

function Just() {}

Just.extend = function(Child, methods) {
	function J() { this.constructor = Child; }
	J.prototype = this.prototype;
	Child.prototype = new J;
	Child.extend = this.extend;
	Child.parent = this;

	for (var m in methods) {
		Child.prototype[m] = methods[m];
	}

	return Child;
};

Just.prototype.just = function(method, args) {
	return this.constructor.parent.prototype[method].apply(this, args || []);
};

Just.prototype.justInit = function(args) {
	this.just('constructor', args);
};

this.Just = Just;

})();