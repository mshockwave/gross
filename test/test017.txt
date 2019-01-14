# Copy propagation test
# Adapted from Muchnick, p.357
main
var a, b, c, d, e;
{
	let a <- 1;
	let b <- a;
	let c <- 4 * b;
	if c > b then
		let d <- b + 2
	fi;
	let e <- a + b
}
.