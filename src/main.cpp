#include <mpi.h>
#include <iostream>
#include <chrono>
#include <string>
#include <queue>

#define SEND_TYPE_INIT_TAG 0
#define SEND_QT_NUMBERS_EXPECTED_TAG 1
#define SEND_INIT_NUM_TAG 2

void getType(   int my_rank,
                int comm_sz,
                int& type){
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

int getNumbersExpectedCount(int my_rank, int comm_sz, int& totalNumbers, int* qtNumbersProccesses){
    int numbersExpectedCount;
    if(my_rank == 0){

        std::cin>>totalNumbers;
        std::cout<<"Quantity of numbers : "<<totalNumbers<<std::endl;
                        
        //Existem mais processos do que números
        if(totalNumbers < comm_sz){
            std::cout<<" There are more proccesses than numbers!\n";
                            
            //Aloca 1 número por processo e 0 aos que não forem necessários
            int numbersAssignedCount = 0;
            for(int proccess = 0; proccess < comm_sz; proccess++){
                if(numbersAssignedCount != totalNumbers){
                    qtNumbersProccesses[proccess] = 1;
                    numbersAssignedCount++;
                }else{
                    qtNumbersProccesses[proccess] = 0;
                }
            }

        }else{//Existem mais números do que processos
            int minimunNumbersPerProccess = (int) totalNumbers/comm_sz;
            std::cout<<"Numbers per Proccess: "<<minimunNumbersPerProccess<<std::endl;

            for(int proccess = 0; proccess < comm_sz; proccess++){
                qtNumbersProccesses[proccess] = minimunNumbersPerProccess;
            }

            //Se todo processo recebeu a mesma quantidade de números
            if(minimunNumbersPerProccess * comm_sz == totalNumbers){
                std::cout<<"Every Proccess will get the same quantity of numbers!\n";

            }else{//Se existem processos com quantidades diferentes de números

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
        //Recebe a quantidade de números esperados para esse processo
        MPI_Recv(&numbersExpectedCount, 1 , MPI_INT, 0, SEND_QT_NUMBERS_EXPECTED_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    return numbersExpectedCount;
}

void getMyNumbers(int my_rank, int comm_sz, int& totalNumbers, std::queue<float>& my_numbers, int* qtNumbersProccesses, int numbersExpected){
    if(my_rank == 0){
        //Block Partition
        int processToSendRank = 0;
        int numbersSentForProccessCount = 0;

        //Recebe todas as entradas
        //Envia para cada processo
        for (int numberReadCount = 0; numberReadCount<totalNumbers; numberReadCount++){

            float numberRead;
            std::cin>>numberRead;
            
            int expectedSentNumberForProccess = qtNumbersProccesses[processToSendRank];
            //Se atingiu a quantidade enviada para o processo, muda o processo
            if (numbersSentForProccessCount == expectedSentNumberForProccess){
                processToSendRank++;
                numbersSentForProccessCount = 0;
            }

            std::cout<<"Enviando "<<numberRead<<" para o processo "<<processToSendRank<<std::endl;

            //Se for o próprio processo 0, adiciona no dele
            if(processToSendRank == 0){
                my_numbers.push(numberRead);
            }else{ //Se não, envia para o processo específico
                MPI_Send(&numberRead, 1, MPI_FLOAT, processToSendRank, SEND_INIT_NUM_TAG, MPI_COMM_WORLD);
            }

            numbersSentForProccessCount++;
        }

    }else{ // Se não for processo 0
        //Recebe os dados específicos dessa entrada
        float numberRead = 0.0;
        for(int numberCount = 0; numberCount < numbersExpected; numberCount++){
            MPI_Recv(&numberRead, 1, MPI_FLOAT, 0, SEND_INIT_NUM_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            my_numbers.push(numberRead);
        }
        
    }
}

void getAllInputs(int my_rank, int comm_sz, int& type, int& totalNumbers, std::queue<float>& my_numbers){
    getType(my_rank, comm_sz, type);
    int qtNumbersProccesses[comm_sz];
    int numbersExpected = getNumbersExpectedCount(my_rank, comm_sz, totalNumbers, qtNumbersProccesses);
    getMyNumbers(my_rank, comm_sz, totalNumbers, my_numbers, qtNumbersProccesses, numbersExpected);
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

   getAllInputs(world_rank, world_size, type, totalNumbers, my_numbers);

   std::cout<<"Proccess "<<world_rank<<" has "<<my_numbers.size()<<" numbers in total!\n";

    MPI_Finalize();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = endTime - initTime;
    auto timeExec = std::chrono::duration_cast<std::chrono::milliseconds>(fp_ms);
    std::cout<<timeExec.count()<<std::endl;

    return 0;
}

