#include <mpi.h>
#include <iostream>
#include <chrono>
#include <string>
#include <stack>

#define SEND_TYPE_INIT_TAG 0
#define SEND_QT_NUMBERS_EXPECTED_TAG 1
#define SEND_INIT_NUM_TAG 2
#define SEND_QT_PROC_WITH_NUMBERS 3
#define SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS 4
#define SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS 5

/**
 * @brief  Get the type of output via users input. "sum" (0) or "time" (1) and send it to all proccesses
 * 
 * @param my_rank in
 * @param comm_sz in
 * @param type out
 */
void getType(int my_rank, int comm_sz, int& type){
    if(my_rank == 0){
        std::string typeRead;
        std::cin>>typeRead;
        
        if(typeRead.compare("sum") == 0){
            type = 0;
        }else if (typeRead.compare("time") == 0){
            type = 1;
        }else{
            type = 2;
        }

        //send type to every other proccess
        for (int proccessRank = 1; proccessRank< comm_sz; proccessRank++){
            MPI_Send(&type, 1 , MPI_INT, proccessRank, SEND_TYPE_INIT_TAG, MPI_COMM_WORLD);
        }
    }else{
        MPI_Recv(&type, 1 , MPI_INT, 0, SEND_TYPE_INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}


/**
 * @brief  Get the quantity of numbers from the user and calculates how many numbers each proccess
 *  will get via block partitioning and send it to each proccess
 * 
 * @param my_rank in
 * @param comm_sz in
 * @param totalNumbers in 
 * @param qtNumbersProccesses out
 * @param qtProccessWithNumbers out
 * @return int 
 */
int getNumbersExpectedCount(int my_rank, int comm_sz, int& totalNumbers, int* qtNumbersProccesses, int& qtProccessWithNumbers){
    int numbersExpectedCount;
    if(my_rank == 0){

        std::cin>>totalNumbers;
        std::cout<<"Quantity of numbers : "<<totalNumbers<<std::endl;
                        
        //There are more proccesses than numbers
        if(totalNumbers < comm_sz){
            qtProccessWithNumbers = totalNumbers;

            std::cout<<" There are more proccesses than numbers!\n";
                            
            //Set each proccess with one number until it reaches the necessary amount.
            //Set 0 numbers for proccesses if needed
            int numbersAssignedCount = 0;
            for(int proccess = 0; proccess < comm_sz; proccess++){
                if(numbersAssignedCount != totalNumbers){
                    qtNumbersProccesses[proccess] = 1;
                    numbersAssignedCount++;
                }else{
                    qtNumbersProccesses[proccess] = 0;
                }
            }

        }else{//There are more numbers than proccesses
            qtProccessWithNumbers = comm_sz;

            int minimunNumbersPerProccess = (int) totalNumbers/comm_sz;
            std::cout<<"Numbers per Proccess: "<<minimunNumbersPerProccess<<std::endl;
            
            //Assign the minimun amount of numbers that each proccess will get
            for(int proccess = 0; proccess < comm_sz; proccess++){
                qtNumbersProccesses[proccess] = minimunNumbersPerProccess;
            }

            //If all proccesses got the same amout of numbers
            if(minimunNumbersPerProccess * comm_sz == totalNumbers){
                std::cout<<"Every Proccess will get the same quantity of numbers!\n";

            }else{//There will be proccesses with more numbers than others

                std::cout<<"Every Proccess will get at least "<<minimunNumbersPerProccess<<" numbers!\n";

                //Add one number count to each proccess until all leftovers were assigned
                int qtLeftOverNumbers = totalNumbers - minimunNumbersPerProccess * comm_sz;
                int assignedLeftOverNumbersCount = 0;

                for(int proccessRank = 0; proccessRank < comm_sz; proccessRank++){
                    qtNumbersProccesses[proccessRank]++;
                    assignedLeftOverNumbersCount++;

                    if(assignedLeftOverNumbersCount == qtLeftOverNumbers){
                        break;
                    }
                }

            }
        }

        std::cout<<"Quantity of proccesses with numbers: "<<qtProccessWithNumbers<<std::endl;
        
        //Send to every proccess the amount of proccesses that have numbers assigned
        for (int proccessRank = 1; proccessRank< comm_sz; proccessRank++){
            MPI_Send(&qtProccessWithNumbers, 1 , MPI_INT, proccessRank, SEND_QT_PROC_WITH_NUMBERS, MPI_COMM_WORLD);
        }

        //Send to every proccess the amount of numbers it should expect
        numbersExpectedCount = qtNumbersProccesses[0];
        for (int proccessRank = 1; proccessRank< comm_sz; proccessRank++){
            MPI_Send(&qtNumbersProccesses[proccessRank], 1 , MPI_INT, proccessRank, SEND_QT_NUMBERS_EXPECTED_TAG, MPI_COMM_WORLD);
        }
    }else{

        //get amount of proccesses that will be assigned numbers
        //may be fewer than the amount of proccesses available
        MPI_Recv(&qtProccessWithNumbers, 1 , MPI_INT, 0, SEND_QT_PROC_WITH_NUMBERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //Get the amount of numbers that the proccess will haves
        MPI_Recv(&numbersExpectedCount, 1 , MPI_INT, 0, SEND_QT_NUMBERS_EXPECTED_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    return numbersExpectedCount;
}

/**
 * @brief Fill the proccess's queue with the amount of numbers expected via block partitioning
 * while reading the user's input 
 * 
 * @param my_rank in
 * @param comm_sz in
 * @param totalNumbers in 
 * @param my_numbers out
 * @param qtNumbersProccesses in 
 * @param numbersExpected in
 */
void getMyNumbers(int my_rank, int comm_sz, int& totalNumbers, std::stack<float>& my_numbers, int* qtNumbersProccesses, int numbersExpected){
    
    if(my_rank == 0){
        //Block Partition
        int processToSendRank = 0;
        int numbersSentForProccessCount = 0;

        //Get all entries and send then to corresponding proccess
        for (int numberReadCount = 0; numberReadCount<totalNumbers; numberReadCount++){

            float numberRead;
            std::cin>>numberRead;
            
            int expectedSentNumberForProccess = qtNumbersProccesses[processToSendRank];
            //If the amount sent for proccess its already what it should have gotten
            if (numbersSentForProccessCount == expectedSentNumberForProccess){
                processToSendRank++;
                numbersSentForProccessCount = 0;
            }

            //If its proccess 0, just push it to its queue
            if(processToSendRank == 0){
                my_numbers.push(numberRead);
            }else{ //Send the number to proccess otherwise
                MPI_Send(&numberRead, 1, MPI_FLOAT, processToSendRank, SEND_INIT_NUM_TAG, MPI_COMM_WORLD);
            }

            numbersSentForProccessCount++;
        }

    }else{
        //Get assigned numbers and push then to queue
        float numberRead = 0.0;
        for(int numberCount = 0; numberCount < numbersExpected; numberCount++){
            MPI_Recv(&numberRead, 1, MPI_FLOAT, 0, SEND_INIT_NUM_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            my_numbers.push(numberRead);
        }
        
    }
}

/**
 * @brief 
 * Gets all inputs from the user and do block partitioning of numbers.
 * Each proccess will have it's own numbers in the queue. A proccess may not be assigned any number
 * if the amount of numbers is higher than proccesses
 * 
 * @param my_rank in
 * @param comm_sz in
 * @param type out
 * @param totalNumbers out 
 * @param my_numbers out
 * @param qtProccessWithNumbers out 
 */
void getAllInputs(int my_rank, int comm_sz, int& type, int& totalNumbers, std::stack<float>& my_numbers, int& qtProccessWithNumbers){
    
    //if there are more proccesses than numbers
    getType(my_rank, comm_sz, type);
    int qtNumbersProccesses[comm_sz];
    int amountNumbersExpected = getNumbersExpectedCount(my_rank, comm_sz, totalNumbers, qtNumbersProccesses, qtProccessWithNumbers);
    getMyNumbers(my_rank, comm_sz, totalNumbers, my_numbers, qtNumbersProccesses, amountNumbersExpected);
}

bool willHaveAPairProccess(const int my_rank, const int comm_sz, const int numberOfProccessesTakenAcount){
    //if there is a pair amount of proccesses
    if(numberOfProccessesTakenAcount % 2 == 0){
        return true;
    }
    //there are odd number of proccesses
    

    //if it is not the last proccess (as it cant have a pair proccess)
    if(my_rank != numberOfProccessesTakenAcount - 1){
        return true;
    }

    //it is the last odd proccess
    std::cout<<"Proccess "<<my_rank<<" will not have a pair proccess!\n";
    return false;

}

/**
 * @brief Get the Pair Proccess Rank. If my proccess rank is odd, its the even rank right below.
 * If my proccess rank is even, its the odd rank right above
 * 
 * @param world_rank My rank
 * @return int 
 */
int getMyPairProccessRank(const int world_rank){
    if(world_rank % 2 == 0){
        //My pair proccess its the next rank
        return world_rank + 1;
    }else{
        //My pair proccess its the previous rank
        return world_rank - 1;
    }
}

/**
 * @brief Get how many numbers my pair proccess has as my proccess send my amount of numbers as well
 * 
 * 
 * @param world_rank My proccess rank
 * @param pairProccessRank My pair proccess rank
 * @param myQtNumbers How many numbers my proccess has
 * @return int How many numbers my pair proccess has
 */
int getQtNumbersPairProccess(const int world_rank, const int pairProccessRank, int myQtNumbers){
    int qtNumbersPair = 0;
    if(world_rank % 2 == 0){
        MPI_Recv(&qtNumbersPair, 1, MPI_INT, pairProccessRank, SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(&myQtNumbers, 1, MPI_INT, pairProccessRank, SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
    }else{
        MPI_Send(&myQtNumbers, 1, MPI_INT, pairProccessRank, SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
        MPI_Recv(&qtNumbersPair, 1, MPI_INT, pairProccessRank, SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    return qtNumbersPair;
}

/**
 * @brief Wait for pairProccessRank to send a number. When it gets it, sum it with a number in the stack and push it
 * 
 * @param pairProccessRank in 
 * @param my_numbers in
 */
void receiveNumberAndPush(const int pairProccessRank, std::stack<float>& my_numbers){
    float numberReceived = 0;
    MPI_Recv(&numberReceived, 1, MPI_FLOAT, pairProccessRank, SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    float numberToSumWith = my_numbers.top();
    my_numbers.pop();
    my_numbers.push(numberToSumWith + numberReceived);
}

void sumUntilItCan(const int world_rank, const int pairProccessRank, int qtNumbersPair, std::stack<float>& my_numbers){
    bool stillHaveNumbersToSend = my_numbers.size() != 1;
    bool stillHaveNumbersToReceive = qtNumbersPair != 1;

    while(stillHaveNumbersToReceive || stillHaveNumbersToSend){
        
        float numberToSend = 0;
        if(stillHaveNumbersToSend){
            //std::cout<<"Proccess "<<world_rank<<" still has numbers to send to"<<pairProccessRank<<std::endl;
            numberToSend = my_numbers.top();
            my_numbers.pop();
        }

        //First part of communication
        if(world_rank % 2 == 0){
            if(stillHaveNumbersToReceive){
                receiveNumberAndPush(pairProccessRank, my_numbers);
                qtNumbersPair--;
            }else{
                //std::cout<<"Proccess "<<world_rank<<" has already received all it's numbers from "<<pairProccessRank<<std::endl;
            }
        }else{
            if(stillHaveNumbersToSend){
                //std::cout<<"Proccess "<<world_rank<<" sending number to Proccess "<<pairProccessRank<<std::endl;
                MPI_Send(&numberToSend, 1, MPI_INT, pairProccessRank, SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
            }else{
                //std::cout<<"Proccess "<<world_rank<<" has already sent all it's numbers from"<<pairProccessRank<<std::endl;
            }
        }
        
        //Second part of communication
        if(world_rank % 2 == 0){
            if(stillHaveNumbersToSend){
                //std::cout<<"Proccess "<<world_rank<<" sending number to Proccess "<<pairProccessRank<<std::endl;
                MPI_Send(&numberToSend, 1, MPI_INT, pairProccessRank, SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
            }else{
                //std::cout<<"Proccess "<<world_rank<<" has already sent all it's numbers from"<<pairProccessRank<<std::endl;
            }
        }else{
            if(stillHaveNumbersToReceive){
                receiveNumberAndPush(pairProccessRank, my_numbers);
                qtNumbersPair--;
            }else{
                //std::cout<<"Proccess "<<world_rank<<" has already received all it's numbers from "<<pairProccessRank<<std::endl;
            }
        }

        stillHaveNumbersToSend = my_numbers.size() != 1;
        stillHaveNumbersToReceive = qtNumbersPair != 1;
    }
    
}


int main(int argc, char** argv){

    auto initTime = std::chrono::high_resolution_clock::now();

    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    std::stack<float> my_numbers;
    int type = 0;
    int totalNumbers = 0;
    int qtProccessWithNumbers = 0;

    getAllInputs(world_rank, world_size, type, totalNumbers, my_numbers, qtProccessWithNumbers);

    //Se o processo atual contém numeros
    if (my_numbers.size() != 0){

        //Se for possível formar um par com ele
        if(willHaveAPairProccess(world_rank, world_size, qtProccessWithNumbers)){
            int pairProccessRank = getMyPairProccessRank(world_rank);

            std::cout<<"Pair assigned: "<<world_rank<<" -- "<<pairProccessRank<<"\n";

            //O número par espera primeiro e o ímpar envia primeiro. Depois o contrário
            
            int qtNumbersPair = 0;
            int myQtNumbers = my_numbers.size();

            qtNumbersPair = getQtNumbersPairProccess(world_rank, pairProccessRank, myQtNumbers);

            sumUntilItCan(world_rank, pairProccessRank, qtNumbersPair, my_numbers);

            std::cout<<"Proccess "<<world_rank<<" has "<<my_numbers.size()<<" numbers in its stack!\n";

            if(my_numbers.size() == 1){
                std::cout<<"Proccess "<<world_rank<<" has "<<my_numbers.top()<<" in its stack!\n";
            }
            //until both have only one number
            if(world_rank % 2 == 0){
                //recv
            }else{
                //send
            }

        }else{
            //Se o processo sobrar, após todo processo trocar informação com seu par
            //ele deve enviar um número para cada processo


        }
    }

    //Reduction phase

    MPI_Finalize();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = endTime - initTime;
    auto timeExec = std::chrono::duration_cast<std::chrono::milliseconds>(fp_ms);
    std::cout<<timeExec.count()<<std::endl;

    return 0;
}

