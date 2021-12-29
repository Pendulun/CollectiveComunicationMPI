#include <mpi.h>
#include <iostream>
#include <chrono>
#include <string>
#include <stack>

#define SEND_TYPE_INIT_TAG 0
#define SEND_TOTAL_NUMBERS 7
#define SEND_QT_NUMBERS_EXPECTED_TAG 1
#define SEND_INIT_NUM_TAG 2
#define SEND_QT_PROC_WITH_NUMBERS 3
#define SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS 4
#define SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS 5
#define SEND_LEFTOVER_PROCCESS_NUMBER_TO_SUM 6

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
        
        //Send totalNumbers to every proccess
        for(int rankProccess = 1; rankProccess < comm_sz; rankProccess++){
            MPI_Send(&totalNumbers, 1 , MPI_INT, rankProccess, SEND_TOTAL_NUMBERS, MPI_COMM_WORLD);
        }
                        
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
        //Get totalNumbers from input
        MPI_Recv(&totalNumbers, 1 , MPI_INT, 0, SEND_TOTAL_NUMBERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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

/**
 * @brief Calculates if this proccess will have a pair proccess to send and receive numbers or
 * if it will be a leftover proccess
 * 
 * @param my_rank 
 * @param comm_sz 
 * @param numberOfProccessesTakenAcount 
 * @return true 
 * @return false 
 */
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
 * @brief Get how many numbers my pair proccess has. My proccess will send my amount of numbers too
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
 * @brief Wait for proccessToReceiveFromRank to send a number. When it gets it, sum it with a number in the stack and push it
 * 
 * @param pairProccessRank in 
 * @param my_numbers in
 * @param tag in
 */
void receiveNumberAndPush(const int proccessToReceiveFromRank, std::stack<float>& my_numbers, const int tag){
    float numberReceived = 0;
    MPI_Recv(&numberReceived, 1, MPI_FLOAT, proccessToReceiveFromRank, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    float numberToSumWith = my_numbers.top();
    my_numbers.pop();
    my_numbers.push(numberToSumWith + numberReceived);
}

/**
 * @brief Given a pair proccess to this proccess, they will comunicate and sum its numbers in a cross way
 * until there is only one number left on each proccess
 * 
 * @param world_rank 
 * @param pairProccessRank 
 * @param qtNumbersPair 
 * @param my_numbers 
 */
void sumWhileItCan(const int world_rank, const int pairProccessRank, int qtNumbersPair, std::stack<float>& my_numbers){
    bool stillHaveNumbersToSend = my_numbers.size() != 1;
    bool stillHaveNumbersToReceive = qtNumbersPair != 1;

    while(stillHaveNumbersToReceive || stillHaveNumbersToSend){
        
        float numberToSend = 0;
        if(stillHaveNumbersToSend){
            numberToSend = my_numbers.top();
            my_numbers.pop();
        }

        //First part of communication
        if(world_rank % 2 == 0){
            if(stillHaveNumbersToReceive){
                receiveNumberAndPush(pairProccessRank, my_numbers, SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS);
                qtNumbersPair--;
            }
        }else{
            if(stillHaveNumbersToSend){
                MPI_Send(&numberToSend, 1, MPI_INT, pairProccessRank, SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
            }
        }
        
        //Second part of communication
        if(world_rank % 2 == 0){
            if(stillHaveNumbersToSend){
                MPI_Send(&numberToSend, 1, MPI_INT, pairProccessRank, SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
            }
        }else{
            if(stillHaveNumbersToReceive){
                receiveNumberAndPush(pairProccessRank, my_numbers, SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS);
                qtNumbersPair--;
            }
        }

        stillHaveNumbersToSend = my_numbers.size() != 1;
        stillHaveNumbersToReceive = qtNumbersPair != 1;
    }
    
}

/**
 * @brief Calculates, given a leftover proccess, if this proccess should receive any number from it
 * 
 * @param my_rank 
 * @param qtNumbersLeftOverProccessHasToSend 
 * @return true 
 * @return false 
 */
bool shouldReceiveNumberFromLeftOverProccess(const int my_rank, const int qtNumbersLeftOverProccessHasToSend){
    return qtNumbersLeftOverProccessHasToSend >= 1 && my_rank < qtNumbersLeftOverProccessHasToSend;
}

/**
 * @brief Calculates, given a leftover proccess, how many numbers it should send to other processes
 * 
 * @param totalAmountNumbers 
 * @param qtProccessWithNumbers 
 * @return int 
 */
int calculateHowManyNumbersLeftOverProcHasToSend(const int totalAmountNumbers, const int qtProccessWithNumbers){
    //The total amount of numbers it has is the minimun
    int qtNumbersLeftOverProccess = (int) totalAmountNumbers/qtProccessWithNumbers;

    //It will send all but one
    return qtNumbersLeftOverProccess - 1;
}

/**
 * @brief Calculates, given a leftover proccess, how many numbers this proccess should receive from it
 * 
 * @param qtNumbersLeftOverProccessHasToSend 
 * @param qtProccessWithNumbers 
 * @param myRank 
 * @return int 
 */
int calculateHowManyNumbersToRecFromLeftOverProccess(const int qtNumbersLeftOverProccessHasToSend,
                                                    const int qtProccessWithNumbers,
                                                    const int myRank){

    int qtNumberToReceiveFromLeftOverProccess = (int) qtNumbersLeftOverProccessHasToSend/(qtProccessWithNumbers - 1);
    int numbersLeft = qtNumbersLeftOverProccessHasToSend % (qtProccessWithNumbers - 1);

    if(myRank < numbersLeft){
        qtNumberToReceiveFromLeftOverProccess++;
    }

    return qtNumberToReceiveFromLeftOverProccess;
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

    if (my_numbers.size() != 0){

        if(willHaveAPairProccess(world_rank, world_size, qtProccessWithNumbers)){
            int pairProccessRank = getMyPairProccessRank(world_rank);
            
            int qtNumbersPair = 0;
            int myQtNumbers = my_numbers.size();

            qtNumbersPair = getQtNumbersPairProccess(world_rank, pairProccessRank, myQtNumbers);

            sumWhileItCan(world_rank, pairProccessRank, qtNumbersPair, my_numbers);

            bool thereIsLeftOverProccess = qtProccessWithNumbers % 2 != 0;
            if(thereIsLeftOverProccess){

                int qtNumbersLeftOverProccessHasToSend = calculateHowManyNumbersLeftOverProcHasToSend(totalNumbers, qtProccessWithNumbers);
                if(shouldReceiveNumberFromLeftOverProccess(world_rank, qtNumbersLeftOverProccessHasToSend)){

                    int qtNumberToReceiveFromLeftOverProccess = calculateHowManyNumbersToRecFromLeftOverProccess(
                                                                                            qtNumbersLeftOverProccessHasToSend,
                                                                                            qtProccessWithNumbers,
                                                                                            world_rank);

                    int leftOverProccessRank = qtProccessWithNumbers-1;
                    for(int numberReceivedCount = 0; numberReceivedCount < qtNumberToReceiveFromLeftOverProccess; numberReceivedCount++){
                        receiveNumberAndPush(leftOverProccessRank, my_numbers, SEND_LEFTOVER_PROCCESS_NUMBER_TO_SUM);
                    }

                }

            }
            
        }else{
            //Its a leftover proccess. A proccess without a pair proccess

            bool shouldSendToOtherProccesses = my_numbers.size() > 1;
            if(shouldSendToOtherProccesses){
                
                int qtNumbersToSend = my_numbers.size() - 1;

                //All proccesses but this one
                int qtProccessAvailableToReceive = qtProccessWithNumbers - 1;

                for(int numbersSentCount = 0; numbersSentCount < qtNumbersToSend; numbersSentCount++){
                    float numberToSend = my_numbers.top();
                    my_numbers.pop();

                    int proccessRankToReceiveNumber =  numbersSentCount % qtProccessAvailableToReceive;

                    MPI_Send(&numberToSend, 1, MPI_FLOAT, proccessRankToReceiveNumber, 
                                SEND_LEFTOVER_PROCCESS_NUMBER_TO_SUM, MPI_COMM_WORLD);

                }

            }

        }

    }

    if(my_numbers.size() == 1){
        std::cout<<"Proccess "<<world_rank<<" has "<<my_numbers.top()<<" in its stack!\n";
    }

    //Reduction phase

    MPI_Finalize();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = endTime - initTime;
    auto timeExec = std::chrono::duration_cast<std::chrono::milliseconds>(fp_ms);
    std::cout<<timeExec.count()<<std::endl;

    return 0;
}

