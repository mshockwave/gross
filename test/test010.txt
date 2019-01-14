// Basic while statement test
main
var a, b, c;
{
	let a <- 1;
	let b <- 2;
	let c <- 3;
	while a < b do
		let a <- a + 1;
		let c <- c + 1;
		call OutputNum(a);
		while b < c do
		    call OutputNum(c);
			let b <- b + 1
		od
	od;
	call OutputNum(a);
	call OutputNum(b);
	call OutputNum(c)
}.