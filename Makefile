all:
	mpic++ -std=c++17 -Wall -o main main.cpp Customer.cpp Hunter.cpp

run:
	mpirun -np 3 main