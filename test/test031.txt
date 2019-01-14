# Predefined function and procedure test
main
var a, b, c, d;
{
	let a <- call InputNum( );
	call OutputNum( a );
	call OutputNewLine( );
	
	while b > c do
		let d <- call InputNum( );
		
		let c <- c + 1;
		
		call OutputNum( d );
		call OutputNewLine( );
		
		let b <- b + 1
	od
}
.
 