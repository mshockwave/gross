// Nested if statement test
// Added final let statement to test that phi propagation continues after all
// the phi instructions.
main
var a, b, c;
{
	let a <- 1;
	let b <- 2;
	let c <- 3;
	if a > b then
		let a <- a + 1;
		if a < b then
			let a <- a * 4;
			if c != a then
				let c <- a
			fi
		fi
	else
		let b <- b + 5;
		if a < b then
			let a <- a - 2
		else
			let a <- a + 3
		fi
	fi;
	let a <- a + 1;
	call OutputNum(a)
}
.