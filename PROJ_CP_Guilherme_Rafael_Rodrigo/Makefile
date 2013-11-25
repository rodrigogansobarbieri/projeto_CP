# Projeto Final da disciplina de Computacao Paralela
# 
# Makefile
# 
# Arquivo responsavel pela compilacao, execucao e
# limpeza dos arquivos gerados ao compilar/executar os benchmarks.
#
PROGRAM_NAME=bench
CC=gcc
MPICC=mpicc
CFLAGS=
INCLUDE=-I ./src/include
FOPENMP=-fopenmp

all:
	$(CC) $(CFLAGS) $(INCLUDE) ./src/serial.c -o ./bin/serial
	$(MPICC) $(CFLAGS) $(INCLUDE) ./src/mpi.c -o ./bin/mpi
	$(CC) $(CFLAGS) $(INCLUDE) ./src/pthreads.c -o ./bin/pthreads -lpthread
	$(CC) $(CFLAGS) $(FOPENMP) $(INCLUDE) ./src/openmp.c -o ./bin/openmp
	$(CC) $(CFLAGS) $(INCLUDE) ./src/bench.c -o ./bin/bench

run:    ${PROGRAM_NAME}
	./bin/bench

clean:
	rm -rf ./bin/* ./tests/mpi/*.out ./tests/openmp/*.out ./tests/pthreads/*.out
