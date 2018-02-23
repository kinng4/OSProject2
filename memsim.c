#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


//struct for the entries
struct entry
{
   int vpn;
   int dirty;
   int present;
   int lru;
};

void initializePageTable(struct entry pageTable[], int size)
{
   int i;
   for(i = 0; i < size; i++)
   {
      pageTable[i].vpn = -1;
      pageTable[i].dirty = 0;
      pageTable[i].present = -1;
   }

}

void initializeCache(struct entry cache[], int numberFrames)
{
   int i;
   
   for(i = 0; i < numberFrames; i++)
   {
      cache[i].vpn = -1;
      cache[i].lru = 0;
   }
}

//fifo function
void fifo(char *fileName, int numberFrames, int debug)
{
   unsigned addr;                      //address from file
   char rw;                            //if it's a read of write operation from the file
   int reads = 0;                      //How many reads have been done
   int writes = 0;                     //How many writes have been done
   int events = 0;                     //How many total events
   int foundinCache = 0;               //To tell if it was found in cache
   int cacheFull = 0;                  //Check if cache is full;
   static int size = 1048567;          //32-bit (2^32) address space / 4 KB (2^12) offset = 2^20 or 1048567 (taken from book)
   unsigned pageNumber;                //Page number
   struct entry pageTable[size];       //page table
   struct entry cache[numberFrames];   //cache table
   int i, j;                           //iterators
   int breakpoint = 0;                     //to break from loops
   
   initializePageTable(pageTable, size);
   initializeCache(cache, numberFrames);
   
   FILE *file;
   file = fopen(fileName, "r");
   if(file)
   {
      while(fscanf(file, "%x %c", &addr, &rw) != EOF)
      {
         events++; //increment the events for each read
      
         pageNumber = addr / 4096;  //address / 4096 to get the offset
         
         //check if it's in the cache
         for(i = 0; i < numberFrames; i++)
         {
            if(cache[i].vpn == pageNumber)
            {
               foundinCache = 1;
               if(rw == 'W')  //if it's a write flag the dirty bit
                  cache[i].dirty = 1; 
               
               if(debug == 1)
                  printf("Found in cache\n");
               
               break;
            }
         }
      
         //if it wasn't found in cache
         if(foundinCache == 0)
         {
            //search for any empty pages
            for( i = 0; i < numberFrames; i++)
            {
               if(cache[i].vpn == -1)
               {
                  reads++; //increment read cause it has to be read from disk if it's not in memory
                  cache[i].vpn = pageNumber; //put the page number into the cache
                  if(rw == 'W')  //if it's a write flag the dirty bit
                  {
                     cache[i].dirty = 1;
                  }
                  
               
                  if(debug == 1)
                     printf("Placed in cache[%d]\n", i);
                     
                  breakpoint = 1;
               } 
               
               if(breakpoint == 1)
                  break;
               
               if(i==(numberFrames-1))
                  cacheFull = 1;  
            }
          }
            
          //if cache was full
          if(cacheFull == 1)
          {
            if(cache[0].dirty == 1)
            {
               pageTable[cache[0].vpn].vpn = cache[0].vpn; //save address to the disk
               if(debug==1)
                  printf("Writing to disk\n");
               writes++;
               cache[i].dirty = 0;
            }
          
             //delete the front entry and add the page to end
             for(i = 0; i < numberFrames; i++)
             {
                if(i < (numberFrames-1))
                {
                   cache[i].vpn = cache[i+1].vpn;
                   cache[i].dirty = cache[i+1].dirty;
                   cache[i].present = cache[i+1].present;
                   cache[i].lru = cache[i+1].lru;
                }
               
                else
                { 
                   reads++; //increment reads since it has to be read from disk
                   cache[i].vpn = pageNumber;
                   //if its write change dirty bit
                   if(rw == 'W')
                     cache[i].dirty = 1;
                   if(rw == 'R')
                   {
                      cache[i].dirty = 0;
                      if(debug==1)
                        printf("Reading from disk\n");
                   }
                }
             }   
          }
          
          //reset flags
          foundinCache = 0;
          cacheFull = 0;
          breakpoint = 0;
       }
        
      printf("Total memory frames: %d\n", numberFrames);
      printf("Events in trace: %d\n", events);
      printf("Total disk reads: %d\n", reads);
      printf("Total disk writes: %d\n", writes);
  }    
}

void lru(char *fileName, int numberFrames, int debug)
{
   unsigned addr;                      //address from file
   char rw;                            //if it's a read of write operation from the file
   int reads = 0;                      //How many reads have been done
   int writes = 0;                     //How many writes have been done
   int events = 0;                     //How many total events
   int foundinCache = 0;               //To tell if it was found in cache
   int cacheFull = 0;                  //Check if cache is full;
   static int size = 1048567;          //32-bit (2^32) address space / 4 KB (2^12) offset = 2^20 or 1048567 (taken from book)
   unsigned pageNumber;                //Page number
   struct entry pageTable[size];       //page table
   struct entry cache[numberFrames];   //cache table
   int i, j;                           //iterators
   int breakpoint = 0;                 //to break from loops
   int maxTime = 0;                    //for lru that has been in there the longest
   int pageReplaced;                   //page to be replaced
   
   initializePageTable(pageTable, size);
   initializeCache(cache, numberFrames);
   
   FILE *file;
   file = fopen(fileName, "r");
   if(file)
   {
      while(fscanf(file, "%x %c", &addr, &rw) != EOF)
      {
         events++; //increment the events for each read
      
         pageNumber = addr / 4096;  //address / 4096 to get the offset
         
         //check if it's in the cache
         for(i = 0; i < numberFrames; i++)
         {
            if(cache[i].vpn == pageNumber)
            {
               foundinCache = 1;
               if(rw == 'W')  //if it's a write flag the dirty bit
                  cache[i].dirty = 1; 
               
               if(debug == 1)
                  printf("Found in cache\n");
               
               cache[i].lru = 0; //if it was found reset the lru time
               
               break;
            }
         }
      
         //if it wasn't found in cache
         if(foundinCache == 0)
         {
            //search for any empty pages
            for( i = 0; i < numberFrames; i++)
            {
               cache[i].lru++; //increment how long its been in the cache
            
               if(cache[i].vpn == -1)
               {
                  reads++; //increment read for reading from disk
                  cache[i].vpn = pageNumber;
                  if(rw == 'W')  //if it's a write flag the dirty bit
                  {
                     cache[i].dirty = 1;
                  }
               
                  if(debug == 1)
                     printf("Placed in cache[%d]\n", i);
                     
                  breakpoint = 1;
               } 
               
               if(breakpoint == 1)
                  break;
               
               if(i==(numberFrames-1))
                  cacheFull = 1;  
            }
            
            //if cache was full
            if(cacheFull == 1)
            {
              //calculate the one with the most lru time
              for(i=0; i < numberFrames; i++)
              {
                 if(cache[i].lru > maxTime)
                 {
                    maxTime = cache[i].lru;
                    pageReplaced = i;
                 }
              } 
            
              if(cache[pageReplaced].dirty == 1)
              {
                pageTable[cache[pageReplaced].vpn].vpn = cache[pageReplaced].vpn; //save address to the disk
                if(debug == 1)
                 printf("Writing to disk\n");
                writes++;
              }
            
              cache[pageReplaced].vpn = pageNumber; //replace the page
              cache[pageReplaced].lru = 0;          //reset the time to 0;
              cache[pageReplaced].dirty = 0;        //reset dirty bit
            
              reads++; //increment read for reading from disk
          
            }
            
          
          }
          
          //reset flags
          foundinCache = 0;
          cacheFull = 0;
          breakpoint = 0;
       }
        
      printf("Total memory frames: %d\n", numberFrames);
      printf("Events in trace: %d\n", events);
      printf("Total disk reads: %d\n", reads);
      printf("Total disk writes: %d\n", writes);
   }
}

void vms(char *fileName, int numberFrames, int debug)
{
   unsigned addr;                      //address from file
   char rw;                            //if it's a read of write operation from the file
   int reads = 0;                      //How many reads have been done
   int writes = 0;                     //How many writes have been done
   int size = 1048567;                 //32-bit (2^32) address space / 4 KB (2^12) offset = 2^20 or 1048567
   
   FILE *file;
   file = fopen(fileName, "r");
   if(file)
   {
   }
}



int main(int argc, char *argv[])
{
   char *fileName;
   int numberFrames;
   char *operation;
   char *debug;
   int flagDebug = 0;
   
   fileName = argv[1];
   numberFrames = atoi(argv[2]);
   operation = argv[3];
   debug = argv[4];
   
   if(argc < 5)
   {
      printf("Please make sure there are 4 arguments\n");
      return 0;
   }
   
   //flag if debug or quiet
   if(strcmp(debug, "debug")==0)
      flagDebug = 1;
   
   else if (strcmp(debug, "quiet")==0)
      flagDebug = 0;
   
   else
   {
      printf("Please enter if it's debug or quiet in the fouth argument\n");
      return 0;
   }
   
   if(strcmp(operation, "fifo")==0)
      fifo(fileName, numberFrames, flagDebug);
   
   else if(strcmp(operation, "lru")==0)
      lru(fileName, numberFrames, flagDebug);
      
   else if(strcmp(operation, "vms")==0)
      vms(fileName, numberFrames, flagDebug);
   
   else
   {
      printf("Please make sure third argument is fifo, lru, or vms\n");
   }
   
   return 0;
}