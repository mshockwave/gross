main
var a, b, c;
{
	let a <- 1;
	let b <- 2;
	let c <- 3;
	while b < c do
		let a <- b + 1;
		while ( b + 1 ) < c do
			while ( b + 2 ) < c do
				let b <- b + 1
			od
		od
	od;
	let b <- c + 1
}
.