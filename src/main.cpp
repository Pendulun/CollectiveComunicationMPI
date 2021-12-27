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

void getAllInputs(int my_rank,
                int comm_sz,
                int& type,
                int& totalNumbers,
                std::queue<float>& my_numbers){

                    getType(my_rank, comm_sz, type);

                    if(my_rank == 0){

                        int numbersPerProccess;
                        int lastProccessQt = 0;
                        std::cin>>totalNumbers;
                        std::cout<<"Quantity of numbers : "<<totalNumbers<<std::endl;
                        
                        if(totalNumbers < comm_sz){
                            std::cout<<" There are more proccesses than numbers!\n";
                            numbersPerProccess = 1;
                        }else{
                            numbersPerProccess = (int) totalNumbers/comm_sz;
                            std::cout<<"Numbers per Proccess: "<<numbersPerProccess<<std::endl;

                            if(numbersPerProccess * comm_sz == totalNumbers){
                                std::cout<<"Every Proccess will get the same quantity of numbers!\n";
                                lastProccessQt = numbersPerProccess;
                            }else{
                                lastProccessQt = totalNumbers - numbersPerProccess * comm_sz;
                                std::cout<<"The last proccess will get :"<< lastProccessQt<<" numbers!\n";
                            }
                        }

                        //Enviar a todo processo a quantidade de números que ele deve esperar
                        for (int proccessRank = 1; proccessRank< comm_sz; proccessRank++){
                            if(proccessRank != comm_sz -1){
                                MPI_Send(&numbersPerProccess, 1 , MPI_INT, proccessRank, SEND_QT_NUMBERS_EXPECTED_TAG, MPI_COMM_WORLD);
                            }else{
                                MPI_Send(&lastProccessQt, 1 , MPI_INT, proccessRank, SEND_QT_NUMBERS_EXPECTED_TAG, MPI_COMM_WORLD);
                            }
                        }

                        //Block Partition
                        int numbersSentPerProccessCount = 0;
                        int totalNumbersSent = 0;
                        int dest = 0;

                        //Recebe todas as entradas
                        //Envia para cada processo
                        for (int i = 0; i<totalNumbers; i++){
                            float numberRead;
                            std::cin>>numberRead;
                            //Se atingiu a quantidade enviada para o processo, muda o processo
                            if (numbersSentPerProccessCount == numbersPerProccess){
                                dest++;
                                numbersSentPerProccessCount = 0;
                            }

                            std::cout<<"Enviando "<<numberRead<<" para o processo "<<dest<<std::endl;

                            //Se for o próprio processo 0, adiciona no dele
                            if(dest == 0){
                                my_numbers.push(numberRead);
                            }else{ //Se não, envia para o processo específico
                                MPI_Send(&numberRead, 1, MPI_FLOAT, dest, SEND_INIT_NUM_TAG, MPI_COMM_WORLD);
                            }

                            numbersSentPerProccessCount++;
                            totalNumbersSent++;

                        }

                    }else{ // Se não for processo 0
                        
                        //Recebe a quantidade de números esperados para esse processo
                        int numbersExpected = 0;
                        MPI_Recv(&numbersExpected, 1 , MPI_INT, 0, SEND_QT_NUMBERS_EXPECTED_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        
                        //Recebe os dados específicos dessa entrada
                        float dadoLido = 0.0;
                        for(int numberCount = 0; numberCount < numbersExpected; numberCount++){
                            MPI_Recv(&dadoLido, 1, MPI_FLOAT, 0, SEND_INIT_NUM_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            my_numbers.push(dadoLido);
                        }
                        
                    }
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

