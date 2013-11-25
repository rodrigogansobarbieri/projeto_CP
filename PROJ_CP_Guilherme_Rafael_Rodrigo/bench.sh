# Projeto Final da disciplina de Computacao Paralela
# 
# bench.sh
# 
# Arquivo responsavel pela execucao dos benchmarks, 
# geracao dos resultados e exibicao dos mesmos.
#
qt_testes=100
qt_benchs=2
cores=4

bench=1
#core=1

#i=0
nproc=4

while [ $bench -le $((qt_benchs)) ]; do
	core=1
	while [ $core -le $((cores)) ]; do
#Coleta dos Resultados
# Arquivos serao gerados com o nome: ./tests/openmp/bench1_2.out (openmp: bench1, 1 core)
/opt/openmpi1.6/bin/mpiexec -n $core ./bin/mpi ./benchmarks/bench$bench.in > ./tests/mpi/bench$bench"_"$core.out
./bin/openmp $core ./benchmarks/bench$bench.in > ./tests/openmp/bench$bench"_"$core.out
./bin/pthreads $core ./benchmarks/bench$bench.in > ./tests/pthreads/bench$bench"_"$core.out

		core=$((core+1))
	done

# cria a pasta tests


# Exibicao dos Resultados

echo "==== Benchmark" $bench "==================================="
echo "Saída"

core=1
while [ $core -le $((cores)) ]; do

while read -r -u 4 linempi && read -r -u 5 linepth && read -r -u 6 lineomp; do

	if [ $core -eq 1 ]; then

	   #read line 
   	   echo -e "$linempi" 
	   echo "#######################################################"
	   echo "Serial"

	   read -r -u 4 linempi && read -r -u 5 linepth && read -r -u 6 lineomp;
	else
		echo "$core cores"
	fi

	echo -e "WallClock (s) \t MPI: $linempi \t| PTHREADS: $linepth \t| OPENMP: $lineomp"
	   read -r -u 4 linempi && read -r -u 5 linepth && read -r -u 6 lineomp;
	echo -e "CPUTime (s) \t MPI: $linempi \t| PTHREADS: $linepth \t| OPENMP: $lineomp"
	   read -r -u 4 linempi && read -r -u 5 linepth && read -r -u 6 lineomp;
	echo -e "Memoria (M) \t MPI: $linempi \t| PTHREADS: $linepth \t| OPENMP: $lineomp" 

	#echo "WallClock (s) " # MPI: 20.001 | PTHREADS: 4.001 | OPENMP: 0.012
	#echo "CPUTime (s) " #­ MPI: 10.010 | PTHREADS: 2.001 | OPENMP: 0.019
	#echo "Memoria (M) " #­ MPI: 1104 | PTHREADS: 2201  | OPENMP: 3120

#	core=$((core+1))
break

done 4<./tests/mpi/bench$bench"_"$core.out 5<./tests/pthreads/bench$bench"_"$core.out 6<./tests/openmp/bench$bench"_"$core.out

	core=$((core+1))

	done

	 bench=$((bench+1))
done
