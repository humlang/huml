

fib0 := 0;
fib1 := 1;

n := read();

for n
{
  old := fib0; 
  fib0 := fib1;
  fib1 := old + fib1;
}

print() := fib1;

