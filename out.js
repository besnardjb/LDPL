

function stdout_render_inner()
{
	var out = $("#OUT2");
	
	out.empty();
		

	out.append("<h3>Aggregated Output</h3>");
	
	var tcontent = "";

	tcontent += "<thead><tr><th>File</th><th>TS</th><th>R</th><th>#</th><th class='wide500'>Line</th></tr></thead>";

	var log = [];

	var rank_filter = /./g;
	
	try
	{
		rank_filter = new RegExp($("#aout_rank").val() ,"g");
	}
	catch( e )
	{
		alert("Bad regular expression " + e );
		rank_filter = /./g;
	}


	var content_filter = /./g;
	
	try
	{
		content_filter = new RegExp($("#aout_content").val() ,"g");
	}
	catch( e )
	{
		alert("Bad regular expression " + e );
		content_filter = /./g;
	}
	
	for( var i = 0 ; i < profile_dat.length ; i++ )
	{
		if( $("#aout_"+i).get(0).checked === false )
		{
			continue;
		}
		
		
		
		for( var j = 0 ; j < profile_dat[i].stdout.length ; j++ )
		{		
			profile_dat[i].stdout[j].name = profile_dat[i].___name;
			profile_dat[i].stdout[j].id = i;
			profile_dat[i].stdout[j].number = profile_dat[i].stdout.length - j;
		}
		
		log = log.concat( profile_dat[i].stdout );
	}

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
	
	for( var k = 0 ; k < log.length ; k++ )
	{
		var l = log[k];
		
		tcontent += "<tr class='line_" + l.id + "'><td>" + l.name + "</td><td>" + to_time( l.time ) + "</td><td>" + l.rank + "</td><td>" + l.number + "</td><td class='consolecontent'>" + l.line + "</td></tr>";
	}
	
	
	out.append("<table class='restable'>" + tcontent + "</table>");
}


function stdout_render_aggregated()
{
	var out = $("#OUT1");
	
	out.empty();


	out.append("<h3> Input Log Control </h3>");
	
	
	for( var i = 0 ; i < profile_dat.length ; i++ )
	{
		out.append( "<input class='aout_control' type='checkbox' id='aout_" + i + "'>" + profile_dat[i].___name + "</input>" );
	}
	
	$(".aout_control").on("change", stdout_render_inner );

	out.append("<h3> Input Rank Control </h3>");
	
	out.append("Rank filter: :");
	out.append("<input id='aout_rank' type='text' value='.'></input>");
	out.append("<button id='aout_rankb' >Apply RegExp</button>");
	
	$("#aout_rankb").on("click", function()
	{
		stdout_render_inner();
	});

	out.append("<h3> Content Control </h3>");
	
	out.append("Content filter: :");
	out.append("<input id='aout_content' type='text' value='.'></input>");
	out.append("<button id='aout_contentb' >Apply RegExp</button>");
	
	$("#aout_contentb").on("click", function()
	{
		stdout_render_inner();
	});



}



function output_render()
{
	
	stdout_render_aggregated();

}

