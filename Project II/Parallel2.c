#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>
/*
 ============================================================================
 Name        : ParallelExercise2.c
 Author      : Aristeidis Angelopoylos
 Version     :
 Copyright   : 
 Description : AM: 141066
 ============================================================================
 */

int main(int argc, char *argv[]) {
    int N=0, Sum=0;
    bool isDiag = true;
    int rank;
    int p, k;
    int Max, Min;
    int *displs = NULL;
    int Pos[2];
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int MaxT[p], MinT[p];
    int RowsPerProc[p], LastPos[p];
    bool isDiagT[p];
    int PosT[p*2];
    int RCounts[p];

    //Get N
    if (rank == 0) {
        printf("Enter the value of N:");
        scanf("%d",&N);
        //Check if we have more processors than Rows
        while (p>N && N==1 && N==0){
            printf("There are more processors than Rows. Enter a different number for N!");
            printf("Enter the value of N:");
            scanf("%d",&N);
        }
    }
    //Broadcast the number of elements
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    int A[N][N];

    //Get the elements
    if (rank == 0) {
        for(int i=0; i<N; i++){
            for(int j=0; j<N; j++){
                printf("Enter the element %d,%d of table A:", i+1,j+1);
                scanf("%d", &A[i][j]);
            }
        }
    }
    //Broadcast table A
    MPI_Bcast(A, N*N, MPI_INT, 0, MPI_COMM_WORLD);

    //Max gets the first diagonal value
    Max = A[0][0];
    if (rank == 0) {
        //Divide Rows to p processors 
        for (int i=0; i<p; i++){
            RowsPerProc[i] = N/p;
        }
        //If there is a remainder add it
        int Remainder = N-((N/p)*p);
        for (int i=0; i<Remainder; i++){
            RowsPerProc[i]++;
        }
        //Calculate the last element of each processor
        for (int i=0; i<p; i++){
            if (i==0){
                LastPos[i]=RowsPerProc[i]-1;
            } else {
                LastPos[i] = LastPos[i-1] + RowsPerProc[i];
            }
        }
        //Calculate the total elements per p
        for (int i=0; i<p; i++){
            RCounts[i] = RowsPerProc[i] * N;
        }
    }
    
    //Broadcast the RowsPerProc table and the LastPos table
    MPI_Bcast(RowsPerProc, p, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(LastPos, p, MPI_INT, 0, MPI_COMM_WORLD);

    /*================================HERE-BE-MAGIC================================*/
    MPI_Barrier(MPI_COMM_WORLD);
    for (int Row=(LastPos[rank]-(RowsPerProc[rank]-1)); Row <= LastPos[rank]; Row++){
        //Create a sum of elements except for the diagonal elements
        for(int Col=0; Col<N; Col++){
            if(Col != Row){
                Sum = Sum + A[Row][Col];
            }
        }
        //Check if diagonal elements are bigger
        if(!(A[Row][Row] > Sum)){
            isDiag = false;
        }
        //Calculate the maximum diagonal
        if(A[Row][Row] > Max){
            Max = A[Row][Row];
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    /*=============================================================================*/
    
    //MPI_C_BOOL works for me using mpicc. If it does not we need to convert from bool to 0 and 1
    MPI_Gather(&isDiag, 1, MPI_C_BOOL, isDiagT, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);
    //Find if table A is actually Diagonal. In this case we do not care about the previous value of isDig on p==0
    if (rank == 0){
        for (int i=0; i<p; i++){
            if (isDiagT[i] == false){
                isDiag = false;
                break;
            }
        }
    }

    if (isDiag == true){
        //Find the final Maximum value of table A
        MPI_Gather(&Max, 1, MPI_INT, MaxT, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (rank == 0){
            Max=MaxT[0];
            for (int i=0; i<p; i++){
                if (MaxT[i] > Max) {
                    Max=MaxT[i];
                }
            }
        }
        MPI_Bcast(&Max, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }

    int B[N * RowsPerProc[rank]], *Bfin;
    /*==============================HERE-BE-MORE-MAGIC==============================*/
    if (isDiag == true){
        MPI_Barrier(MPI_COMM_WORLD);
        //Allocate Memory for tables B, B_fin and displs
        
        //B = malloc( N * RowsPerProc[rank] * sizeof(int));
        Bfin = malloc(N * N * sizeof(int));
        if (rank == 0){
            displs = malloc( p * sizeof(int));
            displs[0] = 0;
           for (int i=1; i<p; i++){
                displs[i] = displs[i-1] + (RowsPerProc[i-1] * N);
            }
        }
        int dis = 0;
        for (int i=0; i<rank; i++){
            dis = dis + RowsPerProc[i];
        }
        //Each p calculates part of B
        for (int Row=0; Row < RowsPerProc[rank]; Row++){
            for(int Col=0; Col<N; Col++){
                if(Row+dis != Col){
                    B[Row * N + Col] = Max - A[Row + dis][Col];
                } else {
                    B[Row * N + Col] = Max;
                }
            }
        }
        //We gather the various B parts and broadcast the final B table
        MPI_Gatherv( B, N*RowsPerProc[rank], MPI_INT, Bfin, RCounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(Bfin, N*N, MPI_INT, 0, MPI_COMM_WORLD);
        //Min gets the first value of B
        Min = B[0];
        Pos[0] = 0;
        Pos[1] = 0;
        //Each p calculates the minimum value of the part of B it is assigned
        for (int Row=(LastPos[rank]-(RowsPerProc[rank]-1)); Row <= LastPos[rank]; Row++){
            for(int Col=0; Col<N; Col++){
                if(Bfin[Row * N + Col] < Min){
                 Min = Bfin[Row * N + Col];
                    Pos[0] = Row;
                    Pos[1] = Col;
                }
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    /*=============================================================================*/

    //Gather the Min Values and the Positions and find the total Minimum
    if (isDiag == true){
        MPI_Gather(&Min, 1, MPI_INT, MinT, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Gather(Pos, 2, MPI_INT, PosT, 2, MPI_INT, 0, MPI_COMM_WORLD);
        if (rank == 0){
            Min=MinT[0];
            for (int i=0; i<p; i++){
                if (MinT[i] < Min) {
                    Min=MinT[i];
                    Pos[0] = PosT[i * RowsPerProc[i]];
                   Pos[1] = PosT[i * RowsPerProc[i] + 1];
                }
            }
        }
    }

    //Finalize
    if (rank == 0) {
        printf("\n");
        printf("\n");
        if (isDiag == true){
            printf("Table IS Diagonal\n");
            printf("The Biggest diagonal element is: %d \n", Max);
            printf("\n");
            printf("======Table B======\n");
            for (int Row=0; Row<N; Row++){
                for (int Col=0; Col<N; Col++){
                    printf(" %d ", Bfin[Row * N + Col]);
                }
                printf("\n");
            }
            printf("\nThe smallest element of B is: %d\n", Min);
            printf("Its position is: %d,%d \n", Pos[0]+1, Pos[1]+1);
        } else {
            printf("Table is not Diagonal\n");
        }
    }
    MPI_Finalize();
    return EXIT_SUCCESS;
}
