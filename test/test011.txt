main
var a, b, c, d;
{
	let a <- 1;
	let b <- 2;
	let c <- 3;
	let d <- 4;
	
	if a< b then
		let a <- a + 1;
		while c < d do
			let c <- c + 2;
			let d <- d + 1
		od
	else
		let a <- a + 2;
		while c < d do
			let c <- c + 3;
			let d <- d + 2
		od
	fi;
	
	while b < d do
		if a > d then
			let b <- c
		else
			let c <- b
		fi
	od
}.