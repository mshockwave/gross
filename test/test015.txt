

main
    var foo, too;
    var af;
    function bar(x);
    var par, q;
    {
        let q <-9;
        let par <- 3;
        while 3 < par do
            if 2 < 3 then
                let q <- par+q
                #let par <- 4
            fi;
            while 4 >= q do
                let q <- par - q
            od;
            let par <- q-3
        od;
        let x <- par+q;
        let af <- 4;
		return x
    };
    function foo(x);
    var par, q;
    {
        let q <-9;
        let par <- 3;
        while 3 < par do
            let q <- par+q;
            while 4 >= q do
                let q <- par - q
            od;
            let par <- q-3
        od;
        let x <- par+q;
		return x
    };
    {
        let foo <- 3+too;
        let af <- foo+6

    }
.
