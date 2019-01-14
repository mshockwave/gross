main
var x, y, z;
array [ 4 ] a;
array [ 4 ][ 4 ] b;
var c;
function foo( );
var i, d;
{
	let i <- 0;
	while i < 10 do
		let y <- y + 2;
		let z <- x + 2;
		let d <- y + z;
		let i <- i + 1
	od;
	return d
};

procedure bar( x, z );
var i, j, e;
{
	let i <- 0;
	let j <- 0;
	while i < 4 do
		while j < 4 do
			let b[ i ][ j ] <- j;
			let j <- j + 1
		od;
		let a[ i ] <- i;
		let i <- i + 1
	od
};

{
	let x <- 0;
	let y <- 0;
	let z <- 0;
	
	call bar( x, z );
	let c <- call foo;
	
	call OutputNum(c)
}.