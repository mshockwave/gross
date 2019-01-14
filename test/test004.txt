main
array[5] input;
var maxnumber;
function max(a1,a2,a3,a4,a5);
array[5] arg;
var size,temp,i;
{
                let arg[1] <- a1;
                let arg[2] <- a2;
                let arg[3] <- a3;
                let arg[4] <- a4;
                let arg[5] <- a5;
                
                let size <- 5;
                let i <- 2;
                let temp <- arg[1];

                while i <= size
                do
                        if arg[i] > temp
                        then
                                let temp <- arg[i]
                        fi;
                        let i <- i+1
                od;
                return temp
        
}
;
{
        let input[1] <- 22;
        let        input[2] <- 61;
        let        input[3] <- 17;
        let        input[4] <- 34;
        let        input[5] <- 11;
        
        let        maxnumber <- call max(input[1],input[2],input[3],input[4])
        
}
.