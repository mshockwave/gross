#Simple parsing test
main
var input;

function factIter(n);
var i, f;
{
	let i <- 1;
	let f <- 1;
	while i <= n do
		let f <- f * i;
		let i <- i + 1
	od;
	return f
};

function factRec(n);
{
	if n <= 1 then
		return 1
	fi;
	return call factRec(n - 1) * n
};

{
	let input <- call InputNum();
	call OutputNum(call factIter(input));
	call OutputNewLine();
	call OutputNum(call factRec(input));
	call OutputNewLine()
}
.