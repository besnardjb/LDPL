

function stdout_render_structured_inner( target )
{
	var out = $("#SOUT2");
	
	out.empty();
		
	out.append("<hr>");
	
	var tcontent = "";

	tcontent += "<thead><tr><th>TS</th><th>R</th><th>#</th><th class='wide500'>Line</th></tr></thead>";

	var log = [];

	var rank_filter = /./g;
	
	try
	{
		rank_filter = new RegExp($("#sout_rank").val() ,"g");
	}
	catch( e )
	{
		alert("Bad regular expression " + e );
		rank_filter = /./g;
	}

	var content_filter = /./g;
	
	try
	{
		content_filter = new RegExp($("#sout_content").val() ,"g");
	}
	catch( e )
	{
		alert("Bad regular expression " + e );
		content_filter = /./g;
	}
	
	var tprof = profile_dat[target];
		
	for( var j = 0 ; j < tprof.stdout.length ; j++ )
	{		
		tprof.stdout[j].number = tprof.stdout.length - j;
	}
		
	log = log.concat( tprof.stdout );
	
	log = log.filter( function(a)
	{ 
		if( (a.rank + "").match(rank_filter) )
			if( (a.line + "").match(content_filter) )
				return 1;
		
		return 0;	
	} );

	log.sort( function( a , b ){
		if( typeof(a.time) == "string" )
			a.time.replace(/,/g, '.');
			
		if( typeof(b.time) == "string" )
			b.time.replace(/,/g, '.');
			
		return parseFloat(a.time) - parseFloat(b.time); 
	});  
	
		
	var output = "";
	
	var md = ( $("#sout_md").get(0).checked === true );
	var mdlb = ( $("#sout_md_lb").get(0).checked === true );
	
	for( var l = 0 ; l < log.length ; l++ )
	{
		var ll = log[l];
		
		if( md )
			if( mdlb)
				output += ll.line + "   \n";			
			else
				output += ll.line + "\n";
		else
			output += ll.line + "<br>";
	}
	
	
	if( md )
	{
		showdown.setOption('tables', '1');
		var converter = new showdown.Converter();
		output = converter.makeHtml(output);
	}

	out.append("<div id='structout'></div>");

	if( !md )
	{
		$("#structout").addClass("consolecontent");
	}
	
	$("#structout").html(output);
}


function stdout_render_structured()
{
	var out = $("#SOUT1");
	
	out.empty();


	out.append("<h3> Input Log Control </h3>");
	
	var optct = "";
	

	for( var i = 0 ; i < profile_dat.length ; i++ )
	{
		optct += "<option value='" + i + "'>" + profile_dat[i].___name + "</option>";
	}
	
	out.append("<select id='sout_log'>" + optct + "</select>");

	$("#sout_log").change( function(){
		var val = this.value;
		stdout_render_structured_inner(val);
	});

	out.append("<h3> Input Rank Control </h3>");
	
	out.append("Rank filter: :");
	out.append("<input id='sout_rank' type='text' value='.'></input>");
	out.append("<button id='sout_rankb' >Apply RegExp</button>");
	
	$("#sout_rankb").on("click", function()
	{
		stdout_render_structured_inner($("#sout_log").val());
	});

	out.append("<h3> Content Control </h3>");
	
	out.append("Content filter: :");
	out.append("<input id='sout_content' type='text' value='.'></input>");
	out.append("<button id='sout_contentb' >Apply RegExp</button>");
	
	$("#sout_contentb").on("click", function()
	{
		stdout_render_structured_inner($("#sout_log").val());
	});

	out.append("<h3>Markdown Control</h3>");

	out.append( "<input id='sout_md' type='checkbox' checked='true'>" );
	out.append("Enable Markdown");
	
	$("#sout_md").on("change", function()
	{
		if( $("#sout_md").get(0).checked === false )
			$(".soutmd").hide();
		else
			$(".soutmd").fadeIn();

		stdout_render_structured_inner($("#sout_log").val());
	});
	

	out.append( "<div class='soutmd'><input id='sout_md_lb' type='checkbox' checked='true'>Force line Breaks" );

	$(".soutmd").on("change", function()
	{
		stdout_render_structured_inner($("#sout_log").val());
	});
	
	
	if( profile_dat.length )
	{
		stdout_render_structured_inner(0);
	}
}



function structured_output_render()
{
	stdout_render_structured();
}

