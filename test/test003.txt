main
array[ 2 ][ 3 ][ 4 ] a;
var b;
var c;
function foo( );
{
   call OutputNum(55);
	let b <- 2;
	let c <- 3;
	return b + c
};
function bar( );
{
call OutputNum(35);
	let b <- 3;
	let c <- 4;
	return b + c
};

{
	let a[ 0 ][ 2 ][ 3 ] <- 1;
	let a[ 0 ][ 2 ][ 2 ] <- 2;
	if a[ 0 ][ 2 ][ 3 ] > a[ 0 ][ 2 ][ 2 ] then
		let a[ 0 ][ 2 ][ 1 ] <- call foo
	else
		let a[ 0 ][ 2 ][ 1 ] <- call foo
		//let a[ 0 ][ 2 ][ 0 ] <- call bar
	fi;

	let b <- a[ 0 ][ 2 ][ 1 ];
	call OutputNum(b)
}.