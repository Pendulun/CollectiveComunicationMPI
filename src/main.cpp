#include <mpi.h>
#include <iostream>

int main(int argc, char** argv){

    MPI_Init(NULL, NULL);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if(world_rank == 0){
        std::cout<< " Hello World from proccess: 0 de "<<world_size<<" processos!\n";
        int rank_recv;
        MPI_Status status;
        for(int i=1; i< world_size; i++){
            MPI_Recv(&rank_recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            std::cout<< " Hello World from proccess: "<<status.MPI_SOURCE<<" de "<<world_size<<" processos!\n";
        }
    }else{
        MPI_Send(&world_rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}

