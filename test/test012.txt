// From Brandis and Mossenbock
// The third line in the then block, the two lines before the if and he 
// lines after the fi are not part of the original test case. 
main
var a, b, c;
{
	let a <- 2;
	let b <- 3;
	if a < b then
		let a <- 1;
		let b <- a + 1;
		let a <- a + 1
	else
		let a <- a + 1;
		let c <- 2
	fi;
	let a <- a + 1;
    call OutputNum(a)
}.