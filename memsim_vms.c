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

int get_current_process(unsigned address)
{
   unsigned quotient;
   int i = 0, temp;
   char p_num[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
   
   quotient = address;
   
   while(quotient!=0) {
		temp = quotient % 16;
		//To convert integer into character
		if( temp < 10)
      temp =temp + 48;
    else
      temp = temp + 55;
      
		p_num[i++]= temp;
		quotient = quotient / 16;
	}
 
   if (p_num[7] == 51){  //if the address begins with 3 (51 in ASCII) then return 1
     return 1;
   }  
   else
     return -1;
}

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
      cache[i].dirty = 0;
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
                     printf("VPN %d placed in cache[%d]\n", pageNumber, i);
                     
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
   int reads = 0;                    //How many reads have been done 
   int writes = 0;                   //How many writes have been done 
   int pageFaultP1 = 0;                //Page was not in P1 memory
   int pageFaultP2 = 0;                //Page was not in P2 memory
   int pageHitP1 = 0;                  //Page was in P1 memory
   int pageHitP2 = 0;                  //Page was in P2 memory
   int events = 0;                     //How many total events
   unsigned pageNumber;                //Page Number
   int size = 1048567;                 //32-bit (2^32) address space / 4 KB (2^12) offset = 2^20 or 1048567
   int rssSize = numberFrames/2;       //Resident set size: maximum number of pages a proccess can keep on memory
   int sizeGlobal = (numberFrames/2) + 1; //Size of two second-chance lists Clean and Dirty.
   struct entry pageTableP1[rssSize];  //Process 1 FIFO list of pages in memory with addresses starting at 3.
   struct entry pageTableP2[rssSize];  //Process 2 FIFO list of pages in memory with addresses other than 3.
   struct entry cleanList[sizeGlobal]; //Second-chance list for clean pages (Read access).
   struct entry dirtyList[sizeGlobal]; //Second-chance list for dirty pages (Write access).
   struct entry memory[numberFrames];  //Memory table.
   int i, j;                           //Iterators
   int foundInFIFO = 0;
   int FIFOfull = 0;
   int memoryNotFound = 0;
   int cleanEmpty = 0;
   int memoryFull = 0;
   int pageOut;                         //Page to kick out from Memory
   int eventsP1 = 0;                    //How many total events for P1
   int eventsP2 = 0;                    //How many total events for P2
   int current_process;                 
   
   initializePageTable(pageTableP1, rssSize);
   initializePageTable(pageTableP2, rssSize);
   initializePageTable(cleanList, sizeGlobal);
   initializePageTable(dirtyList, sizeGlobal);
   initializeCache(memory, numberFrames);
   
   FILE *file;
   file = fopen(fileName, "r");
   if(file)
   {
     while(fscanf(file, "%x %c", &addr, &rw) != EOF)
      {
          events++;
          
          pageNumber = addr / 4096;  //Address / offset
          
          current_process = get_current_process(addr);  //Process 1 if 1 is returned else Process 2.
          
          if(current_process == 1){  //Process 1 issued 
          
            eventsP1++;
            
            for(i = 0; i < rssSize; i++){
            
              if(pageTableP1[i].vpn == pageNumber){
              
                foundInFIFO = 1;    //Page was found in P1 FIFO
                
                pageHitP1++;
                
                //Check if RW access changed, if so update
                if (rw == 'W'){
                  pageTableP1[i].dirty = 1;
                }
                if(debug == 1){
                  printf("Found %d in Process 1 FIFO\n", pageNumber);
                }
                break;
              } 
            }
            
            if(foundInFIFO == 0){    //Page was not found in P1 FIFO
            
              pageFaultP1++;
            
              for(i = 0; i < rssSize; i++){
              
                if(pageTableP1[i].vpn == -1){    //If there is space in FIFO
                  
                  pageTableP1[i].vpn = pageNumber;  //Insert new page
                  
                  if(rw == 'W')  //if it's a write flag the dirty bit
                  {
                     pageTableP1[i].dirty = 1;
                  }
                  
                  if(debug == 1){
                     printf("VPN %d placed in P1[%d]\n", pageNumber, i);
                  }
                  break;
                }
                if(i==(rssSize-1)){  //FIFO for process 1 is full
                  FIFOfull = 1;  
                } 
              } 
              
              if(FIFOfull == 1){
              
                
                if (pageTableP1[0].dirty == 0){    //page_out the oldest page
                
                  for(i = 0; i < sizeGlobal; i++){    //Add to Clean if dirty bit = 0
                    if(cleanList[i].vpn == -1){
                      cleanList[i].vpn = pageTableP1[0].vpn;
                      
                      if(debug == 1){
                      printf("VPN %d placed in Clean[%d]\n", pageTableP1[0].vpn, i);
                      }
                      break;
                    }
                  }    
                }
                else{                                //Add to Dirty if dirty bit = 1
                  for(i = 0; i < sizeGlobal; i++){
                    if(dirtyList[i].vpn == -1){
                      dirtyList[i].vpn = pageTableP1[0].vpn;
                      
                      if(debug == 1){
                      printf("VPN %d placed in Dirty[%d]\n", pageTableP1[0].vpn, i);
                      }
                      break;
                    }
                  }
                } 
               //delete the front entry of FIFO and add the new page to end
               for(i = 0; i < rssSize; i++)
               {
                  if(i < (rssSize-1))
                  {
                     pageTableP1[i].vpn = pageTableP1[i+1].vpn;
                     pageTableP1[i].dirty = pageTableP1[i+1].dirty;
                  }
                  else
                  { 
                     pageTableP1[i].vpn = pageNumber;
                     if(rw == 'W'){
                       pageTableP1[i].dirty = 1;
                     }
                     if(debug==1){
                       printf("VPN %d placed in P1[%d]\n", pageNumber, i); 
                     }
                  }
               }
              }
             } 
             
             for(i = 0; i < numberFrames; i++){
               
               if(memory[i].vpn == pageNumber){    //If new_page is already in memory, then it must be in Clean or Dirty List.
                 
                 for(j = 0; j < sizeGlobal; j++){
                   if (cleanList[j].vpn == pageNumber){  //If new_page was in Clean, remove
                     cleanList[j].vpn = -1;
                     break;
                   }
                   else{
                     if(dirtyList[j].vpn == pageNumber){    //If new_page was in Dirty, remove.
                       dirtyList[j].vpn = -1;
                     }
                   } 
                 }
                 break; 
               }
               if(i==(numberFrames-1)){  //new_page was not found in memory
                     memoryNotFound = 1;  
               } 
             }
             
             if(memoryNotFound == 1){
             
               for(i=0; i<numberFrames; i++){    //If there is space in memory insert new_page in memory
                 if(memory[i].vpn == -1){
                   
                   reads++; //There was space in memory so reads from disk.
                   
                   memory[i].vpn = pageNumber;
                   if(rw == 'W'){
                     memory[i].dirty = 1; //If page is dirty, set the memory frame to dirty.
                   }
                   break;
                 }
                 
                 if(i == (numberFrames-1)){    //Memory is Full
                   memoryFull = 1;
                 }
               }
               
               if(memoryFull == 1){
               
                 for(j = 0; j < sizeGlobal; j++){
                   if (cleanList[j].vpn != -1){  //Remove the first element of Clean if any
                       pageOut = cleanList[j].vpn;  //page to replace in memory
                       cleanList[j].vpn = -1;
                       break;
                   }
                   if(j==(sizeGlobal-1)){  //Clean list is empty
                       cleanEmpty = 1;  
                   }
                 }
                 if(cleanEmpty == 1){
                     for(j = 0; j < sizeGlobal; j++){
                       if (dirtyList[j].vpn != -1){  //Remove the first element of Dirty list
                         pageOut = dirtyList[j].vpn;  //page to replace in memory
                         dirtyList[j].vpn = -1;
                         break;
                       }
                     } 
                 }
                 for(i=0; i < numberFrames; i++){
                   if(memory[i].vpn == pageOut){
                   
                     memory[i].vpn = pageNumber;    //Replace page_out with new_page
                     reads++;                       //Reads from disk since we are replacing a page
                     
                     if(memory[i].dirty == 1){      //If the memory frame was dirty then write to disk
                       writes++;     
                       memory[i].dirty = 0;          //Set memory frame to clean.           
                     }
                     
                     if(rw == 'W'){
                       memory[i].dirty = 1;          //If new_page in memory is dirty set frame to dirty.
                     }
                     break;
                   }
                 }
               }
            }
          } 
          else {    //Process 2 was issued
          
            eventsP2++;
            
            for(i = 0; i < rssSize; i++){
            
              if(pageTableP2[i].vpn == pageNumber){
              
                foundInFIFO = 1;    //Page was found in P2 FIFO
                
                pageHitP2++;
                
                //Check if RW access changed, if so update
                if (rw == 'W'){
                  pageTableP2[i].dirty = 1;
                }
                if(debug == 1){
                  printf("Found %d in Process 2 FIFO\n", pageNumber);
                }
                break;
              } 
            }
            
            if(foundInFIFO == 0){    //Page was not found in P2 FIFO
            
              pageFaultP2++;
            
              for(i = 0; i < rssSize; i++){
              
                if(pageTableP2[i].vpn == -1){    //If there is space in FIFO
                  
                  pageTableP2[i].vpn = pageNumber;  //Insert new page
                  
                  if(rw == 'W')  //if it's a write flag the dirty bit
                  {
                     pageTableP2[i].dirty = 1;
                  }
                  
                  if(debug == 1){
                     printf("VPN %d placed in P2[%d]\n", pageNumber, i);
                  }
                  break;
                }
                if(i==(rssSize-1)){  //FIFO for process 1 is full
                  FIFOfull = 1;  
                } 
              } 
              
              if(FIFOfull == 1){
                
                if (pageTableP2[0].dirty == 0){    //page_out the oldest page
                
                  for(i = 0; i < sizeGlobal; i++){    //Add to Clean if dirty bit = 0
                    if(cleanList[i].vpn == -1){
                      cleanList[i].vpn = pageTableP2[0].vpn;
                      
                      if(debug == 1){
                      printf("VPN %d placed in Clean[%d]\n", pageTableP2[0].vpn, i);
                      }
                      break;
                    }
                  }    
                }
                else{                                //Add to Dirty if dirty bit = 1
                  for(i = 0; i < sizeGlobal; i++){
                    if(dirtyList[i].vpn == -1){
                      dirtyList[i].vpn = pageTableP2[0].vpn;
                      
                      if(debug == 1){
                      printf("VPN %d placed in Dirty[%d]\n", pageTableP2[0].vpn, i);
                      }
                      break;
                    }
                  }
                } 
               //delete the front entry of FIFO and add the new page to end
               for(i = 0; i < rssSize; i++)
               {
                  if(i < (rssSize-1))
                  {
                     pageTableP2[i].vpn = pageTableP2[i+1].vpn;
                     pageTableP2[i].dirty = pageTableP2[i+1].dirty;
                  }
                  else
                  { 
                     pageTableP2[i].vpn = pageNumber;
                     if(rw == 'W'){
                       pageTableP2[i].dirty = 1;
                     }
                     if(debug==1){
                       printf("VPN %d placed in P2[%d]\n", pageNumber, i); 
                     }
                  }
               }
              }
             } 
             
             for(i = 0; i < numberFrames; i++){
               
               if(memory[i].vpn == pageNumber){    //If new_page is already in memory, then it must be in Clean or Dirty List.
                 
                 for(j = 0; j < sizeGlobal; j++){
                   if (cleanList[j].vpn == pageNumber){  //If new_page was in Clean, remove
                     cleanList[j].vpn = -1;
                     break;
                   }
                   else{
                     if(dirtyList[j].vpn == pageNumber){    //If new_page was in Dirty, remove.
                       dirtyList[j].vpn = -1;
                     }
                   } 
                 }
                 break; 
               }
               if(i==(numberFrames-1)){  //new_page was not found in memory
                     memoryNotFound = 1;  
               } 
             }
             
             if(memoryNotFound == 1){
             
               for(i=0; i<numberFrames; i++){    //If there is space in memory insert new_page in memory
                 if(memory[i].vpn == -1){
                   reads++;                     //There is space to insert new page so read from disk.
                   memory[i].vpn = pageNumber;
                   
                   if(rw == 'W'){
                     memory[i].dirty = 1; //If page is dirty, set the memory frame to dirty.
                   }
                   break;
                 }
                 
                 if(i == (numberFrames-1)){    //Memory is Full
                   memoryFull = 1;
                 }
               }
               
               if(memoryFull == 1){
     
                 for(j = 0; j < sizeGlobal; j++){
                   if (cleanList[j].vpn != -1){  //Remove the first element of Clean if any
                       pageOut = cleanList[j].vpn;  //page to replace in memory
                       cleanList[j].vpn = -1;
                       break;
                   }
                   if(j==(sizeGlobal-1)){  //Clean list is empty
                       cleanEmpty = 1;  
                   }
                 }
                 if(cleanEmpty == 1){
                     for(j = 0; j < sizeGlobal; j++){
                       if (dirtyList[j].vpn != -1){  //Remove the first element of Dirty list
                         pageOut = dirtyList[j].vpn;  //page to replace in memory
                         dirtyList[j].vpn = -1;
                         break;
                       }
                     } 
                 }
                 for(i=0; i < numberFrames; i++){
                   if(memory[i].vpn == pageOut){
                     memory[i].vpn = pageNumber;    //Replace page_out with new_page
                     reads++;                       //Reads from disk since we are replacing a page
                     
                     if(memory[i].dirty == 1){      //If the memory frame was dirty then write to disk
                       writes++;     
                       memory[i].dirty = 0;          //Set memory frame to clean.           
                     }
                     
                     if(rw == 'W'){
                       memory[i].dirty = 1;          //If new_page in memory is dirty set frame to dirty.
                     } 
                     break;
                   }
                 }
               }
            }
        } 
      //Reset flags.
       foundInFIFO = 0;
       FIFOfull = 0;
       memoryNotFound = 0;
       cleanEmpty = 0;
       memoryFull = 0;
      }
      printf("Total memory frames: %d\n", numberFrames);
      printf("Total number of events: %d\n", events);
      printf("Events in P1: %d\n", eventsP1);
      printf("Events in P2: %d\n", eventsP2);
      printf("Total disk reads: %d\n", reads);
      printf("Total disk writes: %d\n", writes);
      printf("Total page faults for P1: %d\n", pageFaultP1);
      printf("Total page hits for P1: %d\n", pageHitP1);
      printf("Total page faults for P2: %d\n", pageFaultP2);
      printf("Total page hits for P2: %d\n", pageHitP2);
      
      /*Testing if pages on FIFO for P1 and P2 are fully contained in memory. Best visualized with small num of frames
      for(i=0; i<rssSize; i++){
        printf("%d ", pageTableP1[i].vpn);
      }
      printf("\n");
      
      for(i=0; i<rssSize; i++){
        printf("%d ", pageTableP2[i].vpn);
      }
      printf("\n");
      
      for(i=0; i<numberFrames; i++){
        printf("%d ", memory[i].vpn);
      }
      printf("\n");
      
      for(i=0; i<sizeGlobal; i++){
        printf("%d ", cleanList[i].vpn);
      }
      printf("\n");
      
      for(i=0; i<sizeGlobal; i++){
        printf("%d ", dirtyList[i].vpn);
      }
      printf("\n");
    */
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
