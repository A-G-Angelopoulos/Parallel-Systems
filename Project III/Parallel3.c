#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>
/*
 ============================================================================
 Name        : Parallel Exercise 3
 Author      : Aristeidis Angelopoulos
 Version     :
 Copyright   : 
 Description : AM: 141066. Compile: mpicc s3.c -o s3 
               Run: mpiexec -n <NumberOfProcessors> ./s3
               Make sure the numbers are equally divisible by n processors
 ============================================================================
 */

int main(int argc, char *argv[]) {
   int N = 0, M = 0, Sum = 0, LSum = 0, Count = 0, Counter = 0;
   int rank, p;
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &p);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   FILE *fp;
   char *line = NULL;
   size_t len = 0;
   ssize_t read;
   if (rank == 0) {
      //Open the file
      fp = fopen("/home/Aristidis/Numbers.txt", "r");
      if (fp == NULL) {
         printf("Could not open file. Exiting!\n");
         exit(EXIT_FAILURE);
      }
      //Count how many numbers we have
      while ((read = getline(&line, &len, fp)) != -1) {
         Count++;
      }

      //Check if Numbers are divisible by p 
      if (Count < p) {
         printf("You cannot have more processors than numbers! Quitting\n");
         exit(EXIT_FAILURE);
      }
      if (Count % p != 0) {
         printf("The numbers cannot be equaly divided into %d processors!\n", p);
         exit(EXIT_FAILURE);
      }
      
   }
   MPI_Bcast(&Count, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Barrier(MPI_COMM_WORLD);

   //Create a table to hold said numbers
   int Numbers[Count];
   if (rank == 0) {
      //Get back to the start of the file and store the numbers into the table
      rewind(fp);
      int i = 1;
      fscanf (fp, "%d", &Numbers[0]);
      while (!feof (fp)) {
         fscanf (fp, "%d", &Numbers[i]);
         i++;
      }
      fclose(fp);
      if (line) {
         free(line);
      }
   }

   //Get M and N
   if (rank == 0) {
      printf("Enter the value of M:");
      scanf("%d", &M);
      printf("Enter the value of N:");
      scanf("%d", &N);
      while(M*N != p) {
         printf("M * N must equal the number of processors!\n");
         printf("Enter the value of M:");
         scanf("%d", &M);
         printf("Enter the value of N:");
         scanf("%d", &N);
      }

      //If we only have a single column we rotate it 90 degrees counter clockwise to get a single row.
      //    | 0 |
      //    | 1 |
      //    | 2 |    -->      | 0 | 1 | 2 | 3 | 4 |
      //    | 3 |
      //    | 4 |
      if (M == p){
         int Temp = N;
         N = M;
         M = Temp;
      }
   }
   MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);

   MPI_Barrier(MPI_COMM_WORLD);
   //We create a table and we store the processor numbers into it.
   //e.g. for M=2 and N=3
   //       N   N   N
   //   M | 0 | 1 | 2 |
   //   M | 3 | 4 | 5 |
   // This is the logical order of the calculations
   int Proc[M][N];
   for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++ ) {
         Proc[i][j] = Counter;
         Counter++;
      }
   }
   //We divide n numbers into p processors
   int NumbersPerProc = Count/p;
   //We create a table to hold n/p numbers per processor and we scatter them
   int LocalA[NumbersPerProc];
   MPI_Scatter(Numbers, NumbersPerProc, MPI_INT, LocalA, NumbersPerProc, MPI_INT, 0, MPI_COMM_WORLD);

   MPI_Barrier(MPI_COMM_WORLD);
   //If there are more than one numbers per processor sum them up
   if (NumbersPerProc > 1) {
      for (int i = 0; i < NumbersPerProc; i++) {
         LSum += LocalA[i];
      }
      Sum = LSum;
   }

   MPI_Barrier(MPI_COMM_WORLD);
   Counter = 0;
   if (M > 1 && N > 1) {
      //We start at the end of the array and move up towards the second row
      //(The first row does not have a row above it)
      for (int i = M-1; i >= 1; i--) {
         for (int j = N-1; j >= 0; j--) {
            //Only the processor of the current position will send data
            if (rank == Proc[i][j]) {
               MPI_Send(&Sum, 1, MPI_INT, Proc[i-1][j], Counter, MPI_COMM_WORLD);
            }
            //The processor directly above the current one will receive the data and sum it with the previous data
            if (rank == Proc[i-1][j]) {
               MPI_Recv(&LSum, 1, MPI_INT, Proc[i][j], Counter, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
               Sum += LSum;
            }
            Counter++;
         }
      }
   }

   MPI_Barrier(MPI_COMM_WORLD);
   //Finally we sum up from right to left towards processor 0
   for (int i = N-1; i >= 1; i--) {
      //Current processor sends data
      if (rank == Proc[0][i]) {
         MPI_Send(&Sum, 1, MPI_INT, Proc[0][i-1], Counter, MPI_COMM_WORLD);
      }
      //Previous processor receives data
      if (rank == Proc[0][i-1]) {
         MPI_Recv(&LSum, 1, MPI_INT, Proc[0][i], Counter, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         Sum += LSum;
      }
      Counter++;
   }

   MPI_Barrier(MPI_COMM_WORLD);
   if (rank == 0){
      printf("The sum of the numbers is: %d \n", Sum);
   }
   MPI_Finalize();
   return EXIT_SUCCESS;
}
