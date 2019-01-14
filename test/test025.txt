# Simple CSE test
# (I don't like the ones in Muchnick)
main
var a, b, c, d, e, f, g;
{
	let a <- b + c;
	let d <- b + c;
	let e <- c + b + d;
	
	if a != d then
		let e <- c + b + d;
		let f <- a * d;
		let g <- d * a + 4;
		let b <- 5;
		let e <- c + b + d
	else
		let f <- a / d;
		let g <- d / a
	fi;
	
	let f <- a * d;
	let g <- d / a
}
.