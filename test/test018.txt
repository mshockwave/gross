// Copy propagation test
// Adapted from Muchnick, p.360
main
procedure foo();
var a, b, c, d, e, f, g, h;
{
	let c <- a + b;
	let d <- c;
	let e <- d * d;
	let f <- a + c;
	let g <- e;
	let a <- g + d;
	
	if a < c then
		let h <- g + 1
	else
		let f <- d - g;
		if f > a then
			let c <- 2
		fi
	fi;
	
	let b <- g * a;
	
	call OutputNum(a);
	call OutputNum(b);
	call OutputNum(c);
	call OutputNum(d);
	call OutputNum(f);
	call OutputNum(g);
	call OutputNum(h)
};

{
	call foo
}
.