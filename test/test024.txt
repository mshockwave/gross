# Nested if/while v3
# If I think I pass this, I'm moving on.
# This test should not be attempted by anyone who is pregnant, nursing, has 
# high blood pressure, aliens, and stressed out graduate students.
# Based on test 13.
main
var a, b, c, d, e, f, g, h;
{
	let a <- 1;
	let b <- 2;
	let c <- 3;
	let d <- 4;
	let e <- 5;
	let f <- 6;
	let g <- 7;
	let h <- 8;
	
	while (b / 4 + 5) < 8 do
		let a <- a * 7 + 9;
		if c < d then
			let g <- ( g - 5 ) * h;
			while g > h do
				let h <- h + 1
			od;
			let g <- g + h
		else
			if c >= d then
				let e <- f * f * 7 - 2;
				while (d - 7) != e do
					let d <- d - 1;
					let e <- e + 1
				od;
				let f <- f * e
			else
				let g <- 725;
				while (d - 8) != e do
					let d <- d - 1;
					let e <- e + 1
				od;
				let f <- (g * f) / 4
			fi;
			let g <- g + h
		fi

	od;
	let c <- a * d;
	let h <- g + h - 7;
	let e <- f + b * c
}
.