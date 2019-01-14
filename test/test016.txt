main
var x, y;
procedure foo( );
var a, b;
{
	let a <- 1;
	let b <- 2;
	let x <- a;
	let y <- b
};
procedure bar( a );
var b, c;
{
	let b <- 1;
	let c <- 1;
	let y <- b
};
procedure baz( a, b );
var c, d;
{
	let c <- 1
};
function boo( a, b );
var i;
{
	let i <- 0;
	while i < y do
		let x <- x * x
	od;
	return x + 4
	
};
{
	call foo( );
	call bar( 1 );
	let x <- 3 + 7 - 2;
	let y <- ( 895 * 2 * 2 ) / 2;
	call baz( x, y );
	let y <- y + call boo( 2, 4 )
}
.