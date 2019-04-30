#include <stdio.h>
#include "mpi.h"
#include <stdbool.h>

int main(int argc, char** argv) {
   int rank;
   int p,i,num,temp = 0;
   int source,target;
   int tag1=50, tag2=60, tag3=70;
   //The amount of numbers
   int amount;
   //The numbers
   int data[100];
   //The divided numbers
   int data_loc[100];
   //The amount of numbers for each process
   int Chuncks[p];
   //Per process flag
   bool flag = true;
   //Final Flag; True = Ascending; False = NOT Ascending
   bool finalFlag = true;
   //Menu option selection
   int option = 1;

   MPI_Status status;
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &p);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   while(option != 0) {
      if (rank == 0) {
         printf("Input the amount of numbers:");
         scanf("%d", &amount);
         printf("Input the %d numbers:", amount);
         for (i=0; i < amount; i++){
            scanf("%d", &data[i]);
         }

         //Send the amount of data to the other processes
         for (target = 1; target < p; target++){
            MPI_Send(&amount, 1, MPI_INT, target, tag1, MPI_COMM_WORLD);
         }

         //Calculate the chuncks array for p processes
         for (i=0; i < p; i++) {
            Chuncks[i] = amount / p;
         }
         //Add the remainder
         temp = amount / p;
         temp = amount - (temp * p);
         for (i=0; i < temp; i++) {
            Chuncks[i]++;
         }
         //Print the chuncks per process
         //for (i=0; i < p; i++) {
         //   printf("We have :%d", Chuncks[i]);
         //}
      
         //Keep the first part of the array for process 0
         for (i=0; i < Chuncks[0]; i++) {
            data_loc[i] = data[i];
         }
         //Send the rest of the array parts to the other processes
         num = Chuncks[0]; 
         for (target = 1; target < p; target++) {
            MPI_Send(&data[num], Chuncks[target], MPI_INT, target, tag2, MPI_COMM_WORLD);
            num = num + Chuncks[target];
         }
      } else {
         //Other processes receive the amount of data
         MPI_Recv(&amount, 1, MPI_INT, 0, tag1, MPI_COMM_WORLD, &status);
         //Other processes reveive part of the array
         MPI_Recv(&data_loc[0], Chuncks[rank], MPI_INT, 0, tag2, MPI_COMM_WORLD, &status);
      }

      //Calculate the result here
      int prevNumber = data_loc[0];
      for (i=1; i < Chuncks[rank]; i++) {
         if (prevNumber >= data_loc[i]){
            flag = false;
            break;
         }
      }

      //Send result back to process 0 for printing
      if (rank != 0)  {
         MPI_Send(&flag, 1, MPI_INT, 0, tag3, MPI_COMM_WORLD);
      }
      //Get the results and print them
      else {
         //finalFlag either stays true or becomes false from process 0
         finalFlag = flag;
         printf("\nResult of process %d: %d", rank, flag);
         for (source = 1; source < p; source++)  {
            MPI_Recv(&flag, 1, MPI_INT, source, tag3, MPI_COMM_WORLD, &status);
            printf("\nResult of process %d: %d", source, flag);
            //If any of the flags are false the final flag becomes false
            if ( flag == false ) {
               finalFlag = flag;
            }
         }
      }

      if ( finalFlag == true ) {
         printf("\n\n\nThe array IS ascending");
      } else {
         printf("\n\n\nThe array is NOT ascending");
      }
      if (rank == 0){
         printf("\n\nAll calculations done!\n");
         printf("Continue[1] or Exit[0]\n");
         printf("Choice: ");
         scanf("%d", &option);
         while( option != 0 && option != 1) {
            printf("Wrong choice. Please choose 1 to continue or 0 to exit\n");
            scanf("%d", &option);
         }
      }
   }
  MPI_Finalize(); 
} 


