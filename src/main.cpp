#include <mpi.h>
#include <iostream>
#include <chrono>
#include <string>
#include <stack>
#include <math.h>


enum SendTag {
    SEND_TYPE_INIT_TAG,
    SEND_TOTAL_NUMBERS,
    SEND_QT_NUMBERS_EXPECTED_TAG,
    SEND_INIT_NUM_TAG,
    SEND_QT_PROC_WITH_NUMBERS,
    SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS,
    SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS,
    SEND_LEFTOVER_PROCCESS_NUMBER_TO_SUM,
    SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS_REDUCTION_PHASE,
    SEND_LEFTOVER_PROCCESS_NUMBER_TO_SUM_REDUCTION_PHASE
};

enum TypeFlag{
    SUM,
    TIME,
    ALL
};

/**
 * @brief  Get the type of output via users input. "sum" (0) or "time" (1) and send it to all proccesses
 * 
 * @param myRank in
 * @param comm_sz in
 * @param type out
 */
void getType(const unsigned int myRank, const unsigned int comm_sz, TypeFlag& type){
    if(myRank == 0){
        std::string typeRead;
        std::cin>>typeRead;
        
        if(typeRead.compare("sum") == 0){
            type = TypeFlag::SUM;
        }else if (typeRead.compare("time") == 0){
            type = TypeFlag::TIME;
        }else{
            type = TypeFlag::ALL;
        }

        //send type to every other proccess
        for (int proccessRank = 1; proccessRank< comm_sz; proccessRank++){
            MPI_Send(&type, 1 , MPI_INT, proccessRank, SendTag::SEND_TYPE_INIT_TAG, MPI_COMM_WORLD);
        }
    }else{
        MPI_Recv(&type, 1 , MPI_INT, 0, SendTag::SEND_TYPE_INIT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

/**
 * @brief  Get the quantity of numbers from the user and calculates how many numbers each proccess
 *  will get via block partitioning and send it to each proccess
 * 
 * @param myRank in
 * @param comm_sz in
 * @param totalNumbers in 
 * @param qtNumbersProccesses out
 * @param qtProccessWithNumbers out
 * @return int 
 */
unsigned int getNumbersExpectedCount(const unsigned int myRank, const unsigned int comm_sz, int& totalNumbers, unsigned int* qtNumbersProccesses,
                                        int& qtProccessWithNumbers){
    unsigned int numbersExpectedCount;
    if(myRank == 0){

        std::cin>>totalNumbers;
        std::cout<<"Quantity of numbers : "<<totalNumbers<<std::endl;
        
        //Send totalNumbers to every proccess
        for(int rankProccess = 1; rankProccess < comm_sz; rankProccess++){
            MPI_Send(&totalNumbers, 1 , MPI_INT, rankProccess, SendTag::SEND_TOTAL_NUMBERS, MPI_COMM_WORLD);
        }
                        
        //There are more proccesses than numbers
        if(totalNumbers < comm_sz){
            qtProccessWithNumbers = totalNumbers;

            //std::cout<<" There are more proccesses than numbers!\n";
                            
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

            unsigned int minimunNumbersPerProccess = (unsigned int) totalNumbers/comm_sz;
            //Assign the minimun amount of numbers that each proccess will get
            for(int proccess = 0; proccess < comm_sz; proccess++){
                qtNumbersProccesses[proccess] = minimunNumbersPerProccess;
            }

            //std::cout<<"Numbers per Proccess: "<<minimunNumbersPerProccess<<std::endl;

            bool everyProccessGetSameAmountNumbers = minimunNumbersPerProccess * comm_sz == totalNumbers;
            if(!everyProccessGetSameAmountNumbers){//There will be proccesses with more numbers than others

                //std::cout<<"Every Proccess will get at least "<<minimunNumbersPerProccess<<" numbers!\n";

                //Add one number count to each proccess until all leftovers were assigned
                int qtLeftOverNumbers = totalNumbers - minimunNumbersPerProccess * comm_sz;
                int assignedLeftOverNumbersCount = 0;

                for(int proccessRank = 0; proccessRank < comm_sz; proccessRank++){
                    qtNumbersProccesses[proccessRank]++;
                    assignedLeftOverNumbersCount++;

                    bool assignedAllLeftOvers = assignedLeftOverNumbersCount == qtLeftOverNumbers;
                    if(assignedAllLeftOvers){
                        break;
                    }
                }

            } /* !everyProccessGetSameAmountNumbers*/

        }

        //std::cout<<"Quantity of proccesses with numbers: "<<qtProccessWithNumbers<<std::endl;
        
        //Send to every proccess the amount of proccesses that have numbers assigned
        for (int proccessRank = 1; proccessRank< comm_sz; proccessRank++){
            MPI_Send(&qtProccessWithNumbers, 1 , MPI_INT, proccessRank, SendTag::SEND_QT_PROC_WITH_NUMBERS, MPI_COMM_WORLD);
        }

        //This proccess (0) already knows how much to expect
        numbersExpectedCount = qtNumbersProccesses[0];
        
        //Send to every other proccess the amount of numbers it should expect
        for (int proccessRank = 1; proccessRank< comm_sz; proccessRank++){
            MPI_Send(&qtNumbersProccesses[proccessRank], 1 , MPI_INT, proccessRank, SendTag::SEND_QT_NUMBERS_EXPECTED_TAG, MPI_COMM_WORLD);
        }
    }else{
        //Get totalNumbers from input
        MPI_Recv(&totalNumbers, 1 , MPI_INT, 0, SendTag::SEND_TOTAL_NUMBERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //get amount of proccesses that will be assigned numbers
        //may be fewer than the amount of proccesses available
        MPI_Recv(&qtProccessWithNumbers, 1 , MPI_INT, 0, SendTag::SEND_QT_PROC_WITH_NUMBERS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //Get the amount of numbers that the proccess will haves
        MPI_Recv(&numbersExpectedCount, 1 , MPI_INT, 0, SendTag::SEND_QT_NUMBERS_EXPECTED_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    return numbersExpectedCount;
}

/**
 * @brief Fill the proccess's queue with the amount of numbers expected via block partitioning
 * while reading the user's input 
 * 
 * @param myRank in
 * @param comm_sz in
 * @param totalNumbers in 
 * @param my_numbers out
 * @param qtNumbersProccesses in 
 * @param numbersExpected in
 */
void getMyNumbers(const unsigned int myRank, const unsigned int comm_sz, const int& totalNumbers, std::stack<float>& my_numbers, 
                    unsigned int* qtNumbersProccesses, const unsigned int numbersExpected){
    
    if(myRank == 0){
        //Block Partition
        int processToSendRank = 0;
        int numbersSentForProccessCount = 0;

        //Get all entries and send then to corresponding proccess
        for (unsigned int numberReadCount = 0; numberReadCount<totalNumbers; numberReadCount++){

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
                MPI_Send(&numberRead, 1, MPI_FLOAT, processToSendRank, SendTag::SEND_INIT_NUM_TAG, MPI_COMM_WORLD);
            }

            numbersSentForProccessCount++;
        }

    }else{
        //Get assigned numbers and push then to queue
        float numberRead = 0.0;
        for(unsigned int numberCount = 0; numberCount < numbersExpected; numberCount++){
            MPI_Recv(&numberRead, 1, MPI_FLOAT, 0, SendTag::SEND_INIT_NUM_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
 * @param myRank in
 * @param comm_sz in
 * @param type out
 * @param totalNumbers out 
 * @param myNumbers out
 * @param qtProccessWithNumbers out 
 */
void getAllInputs(const unsigned int myRank, const unsigned int comm_sz, TypeFlag& type, int& totalNumbers,
                     std::stack<float>& myNumbers, int& qtProccessWithNumbers){
    
    //if there are more proccesses than numbers
    getType(myRank, comm_sz, type);
    unsigned int qtNumbersProccesses[comm_sz];
    unsigned int amountNumbersExpected = getNumbersExpectedCount(myRank, comm_sz, totalNumbers, qtNumbersProccesses, qtProccessWithNumbers);
    getMyNumbers(myRank, comm_sz, totalNumbers, myNumbers, qtNumbersProccesses, amountNumbersExpected);
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
int getMyPairProccessRankCrossSumPhase(const int world_rank){
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
        MPI_Recv(&qtNumbersPair, 1, MPI_INT, pairProccessRank, SendTag::SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(&myQtNumbers, 1, MPI_INT, pairProccessRank, SendTag::SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
    }else{
        MPI_Send(&myQtNumbers, 1, MPI_INT, pairProccessRank, SendTag::SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
        MPI_Recv(&qtNumbersPair, 1, MPI_INT, pairProccessRank, SendTag::SEND_NUMBER_AMOUNT_TO_PAIR_PROCCESS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
 * @brief Pop a number from the queue and send it to proccessRankToReceive using the specified tag
 * 
 * @param proccessRankToReceive 
 * @param my_numbers 
 * @param tag 
 */
void popNumberAndSend(const int proccessRankToReceive, std::stack<float>& my_numbers, const int tag){
    float numberToSend = my_numbers.top();
    my_numbers.pop();
    MPI_Send(&numberToSend, 1, MPI_FLOAT, proccessRankToReceive, tag, MPI_COMM_WORLD);     
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
                receiveNumberAndPush(pairProccessRank, my_numbers, SendTag::SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS);
                qtNumbersPair--;
            }
        }else{
            if(stillHaveNumbersToSend){
                MPI_Send(&numberToSend, 1, MPI_FLOAT, pairProccessRank, SendTag::SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
            }
        }
        
        //Second part of communication
        if(world_rank % 2 == 0){
            if(stillHaveNumbersToSend){
                MPI_Send(&numberToSend, 1, MPI_FLOAT, pairProccessRank, SendTag::SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS, MPI_COMM_WORLD);
            }
        }else{
            if(stillHaveNumbersToReceive){
                receiveNumberAndPush(pairProccessRank, my_numbers, SendTag::SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS);
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

/**
 * @brief Given a pair proccess, do the cross sum until both have only one number left
 * 
 * @param world_rank 
 * @param pairProccessRank 
 * @param my_numbers 
 */
void crossSumWithPairProccess(const int world_rank, const int pairProccessRank, std::stack<float>& my_numbers){
    int myQtNumbers = my_numbers.size();
    int qtNumbersPair = getQtNumbersPairProccess(world_rank, pairProccessRank, myQtNumbers);

    sumWhileItCan(world_rank, pairProccessRank, qtNumbersPair, my_numbers);
}

/**
 * @brief Sends each leftover number to a different proccess, so it completes the crossSumPhase
 * 
 * @param my_numbers Numbers of the leftOver proccess in the crossSumPhase 
 * @param qtProccessWithNumbers 
 */
void sendLeftOverNumberCrossSumPhase(std::stack<float>& my_numbers, const int qtProccessWithNumbers){
    int qtNumbersToSend = my_numbers.size() - 1;

    //All proccesses but this one
    int qtProccessAvailableToReceive = qtProccessWithNumbers - 1;

    for(int numbersSentCount = 0; numbersSentCount < qtNumbersToSend; numbersSentCount++){
        float numberToSend = my_numbers.top();
        my_numbers.pop();

        int proccessRankToReceiveNumber =  numbersSentCount % qtProccessAvailableToReceive;

        MPI_Send(&numberToSend, 1, MPI_FLOAT, proccessRankToReceiveNumber, 
                    SendTag::SEND_LEFTOVER_PROCCESS_NUMBER_TO_SUM, MPI_COMM_WORLD);

    }
}

/**
 * @brief Create pairs of proccesses that sum up their numbers until each one has only one number left in its queue.
 * Treats border cases like an odd number of proccesses and pair proccesses with unequal quantity of numbers.
 * 
 * @param world_rank This proccess rank (in)
 * @param my_numbers This proccess numbers (in/out)
 * @param world_size Number of proccesses in total (in)
 * @param qtProccessWithNumbers Number of proccesses that actually have numbers in its queue (in)
 */
void crossSumPhase(const int world_rank,  std::stack<float>& my_numbers, const int world_size,
                     const int qtProccessWithNumbers, const int totalNumbers){

    if (my_numbers.size() != 0){

        if(willHaveAPairProccess(world_rank, world_size, qtProccessWithNumbers)){
            
            int pairProccessRank = getMyPairProccessRankCrossSumPhase(world_rank);
            crossSumWithPairProccess(world_rank, pairProccessRank, my_numbers);

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
                        receiveNumberAndPush(leftOverProccessRank, my_numbers, SendTag::SEND_LEFTOVER_PROCCESS_NUMBER_TO_SUM);
                    }

                }

            }
            
        }else{
            //Its a leftover proccess. A proccess without a pair proccess

            bool shouldSendToOtherProccesses = my_numbers.size() > 1;
            if(shouldSendToOtherProccesses){
                sendLeftOverNumberCrossSumPhase(my_numbers, qtProccessWithNumbers);
            }
        }

    }
}

/**
 * @brief This is the reduction phase. If qtProccessWithNumbers is even, it has log2(qtProccessWithNumbers) iterations. 
 * If qtProccessWithNumbers is odd, it has log2(qtProccessWithNumbers)+1 phase.
 * In the end, the proccess's queue with rank equal to 0 will contain the final result.
 * @param world_rank 
 * @param my_numbers 
 * @param qtProccessWithNumbers 
 */
void reductionPhase(const int world_rank, std::stack<float>& my_numbers, const int qtProccessWithNumbers){
    unsigned int numPhases = (int) log2(qtProccessWithNumbers);

    if(world_rank == 0){
        std::cout<<"qtProccessWithNumbers: "<<qtProccessWithNumbers<<std::endl;
        std::cout<<"Num fases: "<<numPhases<<std::endl;
    }

    unsigned int qtProccessThisPhase = pow(2, numPhases);
    bool thereIsALeftOverProccess = pow(2, numPhases) != qtProccessWithNumbers;
    unsigned int lastProccessRank = qtProccessWithNumbers - 1;

    if(!thereIsALeftOverProccess || (thereIsALeftOverProccess && world_rank != lastProccessRank)){
        for(unsigned int thisPhase = 1; thisPhase <= numPhases; thisPhase ++){
            if(world_rank == 0){
                std::cout<<"Fase de redução: "<<thisPhase<<std::endl;
            }

            //As qtProccessThisPhase is a power of two, this division will always be an integer
            unsigned int halfCountProccessesThisPhase = qtProccessThisPhase/2;

            if(world_rank < halfCountProccessesThisPhase){
                //This proccess is a destiny proccess
                int rankProccessThatWillSendNumber = world_rank + halfCountProccessesThisPhase;
                receiveNumberAndPush(rankProccessThatWillSendNumber, my_numbers, SendTag::SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS_REDUCTION_PHASE);
            }else{
                //This proccess it's an origin proccess
                
                int rankProccessToSendNumber = world_rank - halfCountProccessesThisPhase;
                popNumberAndSend(rankProccessToSendNumber, my_numbers, SendTag::SEND_NUMBER_TO_SUM_TO_PAIR_PROCCESS_REDUCTION_PHASE);
                
                //This proccess will not be in any other phase
                break;
            }
            qtProccessThisPhase = qtProccessThisPhase/2;
        }
    }

    //At this point, only proccess 0 and leftOverProccess (if there is one) will have a number in its queue.
    
    if(thereIsALeftOverProccess){
        if(world_rank == lastProccessRank){
            float numberToSend = my_numbers.top();
            my_numbers.pop();
            MPI_Send(&numberToSend, 1, MPI_FLOAT, 0, SendTag::SEND_LEFTOVER_PROCCESS_NUMBER_TO_SUM_REDUCTION_PHASE, MPI_COMM_WORLD);
        }else if(world_rank == 0){
            receiveNumberAndPush(qtProccessWithNumbers - 1, my_numbers, SendTag::SEND_LEFTOVER_PROCCESS_NUMBER_TO_SUM_REDUCTION_PHASE);
        }
    }
}

int main(int argc, char** argv){
    std::chrono::duration<double, std::milli> totalTimeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                                                                std::chrono::milliseconds(0)
                                                                                            );

    

    MPI_Init(NULL, NULL);

    int worldCommSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldCommSize);

    int myWorldRank;
    MPI_Comm_rank(MPI_COMM_WORLD, &myWorldRank);

    std::stack<float> myNumbers;
    TypeFlag type;
    int totalNumbers = 0;
    int qtProccessWithNumbers = 0;

    auto initTimeInputs = std::chrono::high_resolution_clock::now();
    getAllInputs(myWorldRank, worldCommSize, type, totalNumbers, myNumbers, qtProccessWithNumbers);
    auto finalTimeInputs = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> totalTimeInputs = finalTimeInputs - initTimeInputs;

    std::cout<<"Input time: "<<totalTimeInputs.count()<<std::endl;

    auto initTime = std::chrono::high_resolution_clock::now();
    crossSumPhase(myWorldRank, myNumbers, worldCommSize, qtProccessWithNumbers, totalNumbers);

    reductionPhase(myWorldRank, myNumbers, qtProccessWithNumbers);
    

    if((TypeFlag::SUM == type ||  TypeFlag::ALL == type)  && myWorldRank == 0){
        std::cout<<"SOMA TOTAL: "<<myNumbers.top()<<std::endl;
    }

    MPI_Finalize();

    auto endTime = std::chrono::high_resolution_clock::now();
    //totalTimeElapsed = endTime - initTime - (totalTimeInputs);

    if((TypeFlag::TIME == type ||  TypeFlag::ALL == type) && myWorldRank == 0){
        totalTimeElapsed = endTime - initTime;
        auto timeExec = std::chrono::duration_cast<std::chrono::milliseconds>(totalTimeElapsed);
        std::cout<<timeExec.count()<<std::endl;
    }

    return 0;
}

