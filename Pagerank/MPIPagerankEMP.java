import mpi.MPI;
import java.io.*;
import java.util.*;
import java.util.stream.*;
import java.nio.file.*;
import java.nio.charset.Charset;

// 참고1 : https://lovit.github.io/machine%20learning/2018/04/16/pagerank_and_hits
// 참고2 : https://github.com/patrickwestphal/mpj-express-mvn
/*
		 * mpi_read 수행 과정
		 * - MPJ library functions to perform input file operations.
		 * - MPJ function split/send data to rest of processes and receive data from all other running processes
		 * - Adjacency matrix format along with the nodes that connected through outbound links		 
		
		 * PR 알고리즘 슈도 코드
		 * 1. Read the input file
		 * 2. Store graph of web pages in HashMap
		 * 3. Count the number of URLs in input file
		 * 4. Initialize pagerank values table
		 * 5. Calculate actual page rank value until iteratation is ended
		 * 6. Write the final results to output file
*/

public class MPIPagerankEMP {

    // adjacency matrix read from file
    private HashMap<Integer, ArrayList<Integer>> adjMatrix = new HashMap<Integer, ArrayList<Integer>>();
    private String inputDatapath = "";
    private int iterations = 10;
    private double df = 0.85;
    private int numberofProcess = 1;
    private int rank = 0;
    private int size = 1;
    private int numofLine = 0;

    private double[] localPageRankValues, globalPageRankValue, danglingPageRankValue;

    public void parseArgs(String[] args) {
        inputDatapath = args[3];
        iterations = Integer.parseInt(args[4]);
        df = Double.parseDouble(args[5]);
        numberofProcess = Integer.parseInt(args[1]);
        MPI.Init(args);
        rank = MPI.COMM_WORLD.Rank();
        size = MPI.COMM_WORLD.Size();
    }

    public int getrank() {
        return rank;
    }

    public void initializePageRankVariables() {
        // pagerank 관련 배열 초기화
        globalPageRankValue = new double[numofLine];
        danglingPageRankValue = new double[numofLine];
        localPageRankValues = new double[numofLine];

        for (int i = 0; i < numofLine; i++) {
            double thesize = numofLine;
            globalPageRankValue[i] = 1 / thesize;
            danglingPageRankValue[i] = 0;
            localPageRankValues[i] = 0;
        }
    }

    public void inititalizeAdjMatrix(ArrayList<String> receivedChunk) {
        for (int j = 0; j < receivedChunk.size(); j++) {
            String line = receivedChunk.get(j);
            String split_line[] = line.split(" ");
            if (split_line.length > 1) {
                ArrayList<Integer> edgeInOuts = new ArrayList<>();
                for (int i = 1; i < split_line.length; i++) // i : except for unique pagerank node = info in/out edges
                edgeInOuts.add(Integer.parseInt(split_line[i]));
                adjMatrix.put(Integer.parseInt(split_line[0]), edgeInOuts);
            } else if (split_line.length == 1) // There are no in/out edges per node
                adjMatrix.put(Integer.parseInt(split_line[0]), null);
        }
    }

    /**
     * Read the input from the file and populate the adjacency matrix
     * <p>
     * The input is of type
     * <p>
     * The first value in each line is a URL. Each value after the first value is
     * the URLs referred by the first URL.
     * For example the page represented by the 0 URL doesn't refer any other URL.
     * Page
     * represented by 1 refer the URL 2.
     *
     * @throws java.io.IOException if an error occurs
     */

    public void loadInput() throws IOException {

        try (Stream<String> lines = Files.lines(Paths.get(inputDatapath), Charset.defaultCharset())) {
            numofLine = (int) lines.count();
        }

        initializePageRankVariables();

        try (BufferedReader br = new BufferedReader(new FileReader(inputDatapath))) {
            int chunkSize = numofLine % numberofProcess == 0 ? numofLine / numberofProcess
                    : numofLine / numberofProcess + 1;
            int count = 0;
            int senderIndex = 1; // start from process rank1.
            boolean IsRankZero = false;
            ArrayList<String> rsvStr = new ArrayList<String>();
            Object[] rcvObj = new Object[1];

            if (rank == 0) {
                System.out.println("Chunk Size:" + chunkSize);

                ArrayList<String> chunkNodePerProcess = new ArrayList<String>();
                for (int i = 0; i < numofLine; i++) {
                    // each line adds to chunkNodePerProcess
                    String tempstr = br.readLine();
                    chunkNodePerProcess.add(tempstr); // ex. 0 1 6 3 5 ... 10 16 2 5
                    count++;
                    // Send partitioned pagerank chunk data to rank processes
                    if (count == chunkSize || i == numofLine - 1) {
                        // Init rank0(master process)
                        if (!IsRankZero) {
                            rsvStr.addAll(chunkNodePerProcess);
                            IsRankZero = true;
                        } else {
                            // after initializing rank 0, send the object of array<string> to the process
                            // with the number of rest rank.
                            Object[] chunkNodePerProcessObj = new Object[1];
                            chunkNodePerProcessObj[0] = (Object) chunkNodePerProcess;
                            MPI.COMM_WORLD.Send(chunkNodePerProcessObj, 0, 1, MPI.OBJECT, senderIndex, 1);
                            senderIndex++;
                        }
                        chunkNodePerProcess.clear(); // initialize chunkNodePerProcess set 0 and clear
                        count = 0; // initialize count 0 to collect new chunk of the rest of ranks
                    }
                }
            } else {
                // If rank is not rank 0,
                MPI.COMM_WORLD.Recv(rcvObj, 0, 1, MPI.OBJECT, 0, 1);
            }

            if (rank != 0)
                rsvStr = (ArrayList<String>) rcvObj[0];
            inititalizeAdjMatrix(rsvStr); // Based on rsvStr, try to initialize adj matrix
        }
    }

    public void calculatePageRank() {
        /*
         * 1. First calculate intermediate page rank values for each line(urls)
           2. Distribute average value of dangling line(urls) to each web page
           3. Multiply the summation by damping factor to reduce the pagerank of ranks
        */

        for (int i = 0; i < iterations; i++) {
            for (int j = 0; j < numofLine; j++)
                localPageRankValues[j] = 0.0;
            double danglingContrib = 0.0;
            Iterator it = adjMatrix.entrySet().iterator(); // HashMap<Integer, ArrayList<Integer>>();

            while (it.hasNext()) {
                Map.Entry<Integer, List> pair = (Map.Entry) it.next();

                // If it is a dangling node, PR2[i] = PR1[i] + Dangling_pages_PR / page_cnt
                // 참고 : https://github.com/shrey920/Pagerank
                if (pair.getValue() == null)
                    danglingContrib += globalPageRankValue[pair.getKey()] / numofLine;
                else {
                    int current_size = pair.getValue().size();
                    Iterator iter = pair.getValue().iterator();
                    
                    // For each outbound link for a node
                    while (iter.hasNext()) {
                        int node = Integer.parseInt(iter.next().toString());
                        double temp = localPageRankValues[node];
                        temp += globalPageRankValue[pair.getKey()] / current_size;
                        localPageRankValues[node] = temp;
                    }
                }
            }

            double tempSend[] = new double[1];
            double tempRecv[] = new double[1];
            tempSend[0] = danglingContrib;
            // public void Allreduce(Object sendbuf, int sendoffset, Object recvbuf, int recvoffset, int count, Datatype datatype, Op op)
            // Distribute updated dangling value to each web page 
            MPI.COMM_WORLD.Allreduce(tempSend, 0, tempRecv, 0, 1, MPI.DOUBLE, MPI.SUM);
            // Send the localPageRankValue
            MPI.COMM_WORLD.Allreduce(localPageRankValues, 0, globalPageRankValue, 0, numofLine, MPI.DOUBLE, MPI.SUM);
            
            if (rank == 0) {
                for (int k = 0; k < numofLine; k++) {
                    globalPageRankValue[k] += tempRecv[0];
                    globalPageRankValue[k] = df * globalPageRankValue[k] + (1 - df) * (1.0 / (double) numofLine);
                }
            }

            // broadcast globalPageRankValue after finishing iteration to the rest of subranks.
            MPI.COMM_WORLD.Bcast(globalPageRankValue, 0, numofLine, MPI.DOUBLE, 0);
        }
    }

    /**
     * Print the pagerank values. Before printing you should sort them according to
     * decreasing order.
     * Print all the values to the output file. Print only the first 10 values to
     * console.
     *
     * @throws IOException if an error occurs
     */

    public void printValues() throws IOException {
        for (int k = 0; k < numofLine; k++)
            System.out.println(k + ":" + globalPageRankValue[k]);
        // HashMap<Integer, Double> pageRank = new HashMap<Integer, Double>();
        // for (int i = 0; i < globalPageRankValue.length; i++) {
        //     pageRank.put(i, globalPageRankValue[i]);
        // }

        // Set<Map.Entry<Integer, Double>> set = pageRank.entrySet();

        // List<Map.Entry<Integer, Double>> list = new ArrayList<Map.Entry<Integer, Double>>(set);
        // Collections.sort(list, new Comparator<Map.Entry<Integer, Double>>() {
        //     public int compare(Map.Entry<Integer, Double> o1, Map.Entry<Integer, Double> o2) {
        //         return (o2.getValue()).compareTo(o1.getValue());
        //     }
        // });
    }

    public static void main(String[] args) throws IOException {

        MPIPagerankEMP mpiPrEMP = new MPIPagerankEMP();
        mpiPrEMP.parseArgs(args);
        mpiPrEMP.loadInput();
        mpiPrEMP.calculatePageRank();

        if (mpiPrEMP.getrank() == 0) {
            // Prints final result per each iteration
            mpiPrEMP.printValues();
        }
        MPI.Finalize();
    }
}

