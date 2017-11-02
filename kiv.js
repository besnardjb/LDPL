
var __p = null;


function gen_kiviat_series( paper, data )
{
	
	
}



function gen_kiviat( paper, profiles )
{
	var pcount = profiles.length;

	var i = 0;
	
	for( i = 0 ; i < pcount ; i++ )
	{
		var p = profiles[i];
	
		
		gen_kiviat_series( paper, p );
	}
}




function raph_init()
{
	$('#KDRAW').empty();

	if( __p )
		delete __p;
	
	__p = new Raphael( document.getElementById("KDRAW"), 500, 500 );

 

}




function render_kiviat()
{
	raph_init();
	
	gen_kiviat( __p , profile_dat );
}

