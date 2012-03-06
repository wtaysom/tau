/*
$(document).ready(function() {
	$("a").click(function() {
		alert("Hello world!");
	});
	$("#orderedlist").addClass("yellow");
});

$(document).ready(function() {
	$("#orderedlist").addClass("red");
});
$(document).ready(function() {
	$("#orderedlist > li").addClass("blue");
});
$(document).ready(function() {
	$("#orderedlist li:last").hover(function() {
		$(this).addClass("green");
	},function(){
		$(this).removeClass("green");
	});
});
$(document).ready(function() {
	$("#orderedlist").find("li").each(function(i) {
		$(this).append( " BAM! " + i );
	});
});
$(document).ready(function() {
	// use this to reset a single form
	$("#reset").click(function() {
		$("form")[0].reset();
	});
});
$(document).ready(function() {
	// use this to reset several forms at once
	$("#reset").click(function() {
		$("form").each(function() {
			this.reset();
		});
	});
});
$(document).ready(function() {
	$("li").not(":has(ul)").css("border", "1px solid black"); 
});
$(document).ready(function() {
	$("a[name]").css("background", "#eee" );
});
$(document).ready(function() {
	$("a[href*='/content/gallery']").click(function() {
		// do something with all links that point somewhere to /content/gallery
	});
});
$(document).ready(function() {
	$('#faq').find('dd').hide().end().find('dt').click(function() {
		$(this).next().slideToggle();
	});
});
$(document).ready(function(){
	$("a").hover(function(){
		$(this).parents("p").addClass("highlight");
	},function(){
		$(this).parents("p").removeClass("highlight");
	});
});
$(function() { // short cut form $(document).ready(function() {
	$("a").click(function() {
		alert("Hello world!");
	});
});
*/
$(document).ready(function() {
	// generate markup
	$("#rating").append("Please rate: ");
	
	for ( var i = 1; i <= 5; i++ ) {
		$("#rating").append("<a href='#'>" + i + "</a> ");
	}
	
	// add markup to container and apply click handlers to anchors
	$("#rating a").click(function(e) {
		// stop normal link click
		e.preventDefault();
		
		// send request
		$.post("rate.php", {rating: $(this).html()}, function(xml) {
			// format and output result
			$("#rating").html(
				"Thanks for rating, current average: " +
				$("average", xml).text() +
				", number of votes: " +
				$("count", xml).text()
			);
		});
	});
});

