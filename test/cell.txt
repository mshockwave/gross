main
var ColCount;
var RowCount;
array[202] Data;
var Rule;
array[2][2][2] RuleBin;

function SetNextBit(Last, Akt, Next, Bits);
{
  let RuleBin[Last][Akt][Next] <- Bits - (Bits / 2) * 2;
  return Bits / 2
};

procedure InitRuleBin;
var Bits;
{
  let Bits <- Rule;
  let Bits <- call SetNextBit(0, 0, 0, Bits);
  let Bits <- call SetNextBit(0, 0, 1, Bits);
  let Bits <- call SetNextBit(0, 1, 0, Bits);
  let Bits <- call SetNextBit(0, 1, 1, Bits);
  let Bits <- call SetNextBit(1, 0, 0, Bits);
  let Bits <- call SetNextBit(1, 0, 1, Bits);
  let Bits <- call SetNextBit(1, 1, 0, Bits);
  let Bits <- call SetNextBit(1, 1, 1, Bits)
};

procedure ClearData;
var i;
{
  let i <- 0;
  while i < ColCount + 2 do
    let Data[i] <- 0;
    let i <- i + 1
  od
};


procedure Output;
var i;
{
  let i <- 1;
  while i <= ColCount do
    if Data[i] == 0 then
      call OutputNum(1)
    else 
    	if Data[i] == 1 then
      		call OutputNum(8)
    	else
      		call OutputNum(0)
      	fi
    fi;
    let i <- i + 1
  od;
  call OutputNewLine()
};

procedure CalcNext;
var i;
var Last, Akt, Next;
{
  let Data[0] <- Data[1];
  let Data[ColCount + 1] <- Data[ColCount];
  
  let Last <- Data[0];
  let Akt <- Data[1];
  
  let i <- 1;
  while i <= ColCount do
    let Next <- Data[i + 1];
    let Data[i] <- RuleBin[Last][Akt][Next];
    
    let Last <- Akt;
    let Akt <- Next;
    let i <- i + 1
  od
};
    

procedure Run;
var i;
{
  let i <- 0;
  while i < RowCount do
    call Output();
    call CalcNext();
    let i <- i + 1
  od
};
 
{
  let ColCount <- 200;
  let RowCount <- 150;

  call ClearData();
  let Data[1] <- 1;
  let Data[80] <- 1;
  let Data[100] <- 1;
  let Data[120] <- 1;
  let Data[200] <- 1;
  let Rule <- 110;
  call InitRuleBin();
  
  call Run()
}.