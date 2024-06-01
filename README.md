# Monte Carlo
## Description
This is a program which approximates the value of pi using monte carlo. It creates child processes which proceed calculation independently and communicte using shared memory. Value returned is the average value of all approximations.
## Usage
To run this program, use:
```
make
./main N
```
where N is a number of processes created.
