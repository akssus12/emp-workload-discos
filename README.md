# emp-workload-discos
Evaluation of cyclomatic complexity between EMP and distributed computing

# How to complile
javac -cp $MPJ_HOME/lib/mpj.jar MPIPagerankEMP.java

# How to execute
$MPJ_HOME/bin/mpjrun.sh -np 4 MPIPageRank pagerank.500k 100 0.85
pagerank.500k -> inputFile for pagerank
100 - number of iterations
0.85 - pagerank value for dampling factor
