# Killing a subexpression test
main
var i, j, k, w, z, x, y;
array[ 8 ] a, b;
{
	let x <- a[ i ];
	let k <- a[ i ];
	while a[ i ] < b[ j ] do
		let x <- a[ i ];
		let y <- a[ j ];
		let a[ k ] <- z;
		let z <- b[ j ];
		let z <- a[ j ]
	od; 
	let w <- a[ i ]
}.