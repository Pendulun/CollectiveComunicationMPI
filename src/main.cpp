#include <mpi.h>
#include <iostream>
#include <chrono>
#include <string>
#include <queue>

#define SEND_TYPE_INIT_TAG 0
#define SEND_QT_NUMBERS_EXPECTED_TAG 1
#define SEND_INIT_NUM_TAG 2

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


        //Enviar a todo processo a quantidade de números que ele deve esperar
        numbersExpectedCount = qtNumbersProccesses[0];
        for (int proccessRank = 1; proccessRank< comm_sz; proccessRank++){
            MPI_Send(&qtNumbersProccesses[proccessRank], 1 , MPI_INT, proccessRank, SEND_QT_NUMBERS_EXPECTED_TAG, MPI_COMM_WORLD);
        }
    }else{
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
void getMyNumbers(int my_rank, int comm_sz, int& totalNumbers, std::queue<float>& my_numbers, int* qtNumbersProccesses, int numbersExpected){
    
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

            std::cout<<"Sending "<<numberRead<<" to proccess "<<processToSendRank<<std::endl;

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
void getAllInputs(int my_rank, int comm_sz, int& type, int& totalNumbers, std::queue<float>& my_numbers, int& qtProccessWithNumbers){
    
    //if there are more proccesses than numbers
    getType(my_rank, comm_sz, type);
    int qtNumbersProccesses[comm_sz];
    int amountNumbersExpected = getNumbersExpectedCount(my_rank, comm_sz, totalNumbers, qtNumbersProccesses, qtProccessWithNumbers);
    getMyNumbers(my_rank, comm_sz, totalNumbers, my_numbers, qtNumbersProccesses, amountNumbersExpected);
}

int main(int argc, char** argv){

    auto initTime = std::chrono::high_resolution_clock::now();

    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    std::queue<float> my_numbers;
    int type = 0;
    int totalNumbers = 0;
    int qtProccessWithNumbers = 0;

    getAllInputs(world_rank, world_size, type, totalNumbers, my_numbers, qtProccessWithNumbers);

    std::cout<<"Proccess "<<world_rank<<" has "<<my_numbers.size()<<" numbers in total!\n";

    MPI_Finalize();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = endTime - initTime;
    auto timeExec = std::chrono::duration_cast<std::chrono::milliseconds>(fp_ms);
    std::cout<<timeExec.count()<<std::endl;

    return 0;
}

