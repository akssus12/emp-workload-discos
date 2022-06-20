# emp-workload-discos
Evaluation of cyclomatic complexity between EMP and distributed computing

# JAVA_PR | How to complile
javac -cp $MPJ_HOME/lib/mpj.jar MPIPagerankEMP.java

# JAVA_PR | How to execute
$MPJ_HOME/bin/mpjrun.sh -np 4 MPIPageRank pagerank.500k 100 0.85
pagerank.500k -> inputFile for pagerank
100 - number of iterations
0.85 - pagerank value for dampling factor

# C_WC | How to compile
cd WordCount/c
make -> wc, mpi_wc

# C_WC | How to execute
dataset : [Amazon reviews](https://www.kaggle.com/datasets/bittlingmayer/amazonreviews)

Mode : serial
./wc [dataset]

Mode : MPI
mpirun -np [#core] [--hostfile own_hostfile] mpi_wc [dataset]
* When using a remote machine, the dataset & code must be in the same location.

# C_PR | How to compile
cd Pagerank/c
make -> mpi_pr
make trim -> datatrim
make serial -> pr

# C_PR | How to execute
dataset : [Livejournal social network](http://snap.stanford.edu/data/soc-LiveJournal1.html)

datatrim fetch the subset of the original data.
Outputs
    data_input_meta : first line indicating the number of the nodes, the following lines indicating the node index, number of incoming links, number of outgoing links.

    data_input_link : the directed links (source node -> destination node )

Mode : serial
./pr

Mode : MPI
./mpi_pr
