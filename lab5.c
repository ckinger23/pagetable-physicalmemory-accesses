/*
 * Lab5: Paging System
 * Carter King
 * CS330: Operating Systems
 * Dr. Larkins
 * Due April 26, 2019
 *
 *
 * This Program simulates a Page Table and Physical Memory in tracing memory accesses. This project incorporates the
 * eviction strategies FIFO, Second Chance, and Least Recently Used. It sadly is not able to run Least Recently Used
 *
 * I last worked on this project on Sunday night April 28 when it was due Friday the 26
 *
 * Sources:
 *https://linux.die.net/man/3/fdopen
 *http://www.zentut.com/c-tutorial/c-write-text-file/
 *https://linux.die.net/man/3/getline
 *https://www.quora.com/In-Linux-how-is-a-file-descriptor-and-a-FILE*-converted-back-and-forth
 *http://www.cplusplus.com/reference/cstring/strtok/
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <netdb.h>

//struct for phys. memory frames
struct frames{
  int page;
  int process;
  int inUse;
};

//struct for pages in page table
struct pages{
  //is present in main mem.
  int present;
  //used for second chance
  int reference;
  int frame;
  //used for LFU
  int count;
  //used for LRU
  int time;
  int addedToPhys;
};

//pagetable struct
struct pagetable{
  struct pages *pt;
  int procSize;
  int framesPerProcess;
  int framesInUse;
  int pageFaults;
  int pagesPerProcess;
  int memAccesses;
};


//First In First Out eviction algorithm
void fifo(struct pagetable * pagetables, struct frames * physMem, int whichProc, int whichPage, int evicScope, int counter, int numFrames){ 
  int swapPage, corrFrame;
  int firstIn = -1;
  int physCounter = 0;
  //update pagetable entry
  pagetables[whichProc].pt[whichPage].present = 1;
  //pagetables[whichProc].pt[whichPage].count ++;
  //update count later
  pagetables[whichProc].pt[whichPage].addedToPhys = counter;
  //local
  if(evicScope == 1){
    //if frames for process are full
    if(pagetables[whichProc].framesPerProcess == pagetables[whichProc].framesInUse){
      //loop through all pages to find those in memory
      for(int i = 0; i < pagetables[whichProc].pagesPerProcess; i++){
        //access all pages that are currently in memory
        if(pagetables[whichProc].pt[i].present == 1){
          //initially set firstIn to first page
          if(firstIn == -1){
            swapPage = i;
            firstIn = pagetables[whichProc].pt[i].addedToPhys;
          }
          //compare the pages as you loop
          else{
            if(pagetables[whichProc].pt[i].addedToPhys < firstIn){
              swapPage = i;
              firstIn = pagetables[whichProc].pt[i].addedToPhys;
            }
          }
        }
      }
      //found the first in page to pull out
      pagetables[whichProc].pageFaults ++;
      pagetables[whichProc].memAccesses ++;
      pagetables[whichProc].pt[swapPage].present = 0;
      pagetables[whichProc].pt[whichPage].frame = pagetables[whichProc].pt[swapPage].frame;
      //get the phys. frame to place
      corrFrame = pagetables[whichProc].pt[swapPage].frame;
      pagetables[whichProc].pt[swapPage].frame = -1;
      pagetables[whichProc].pt[swapPage].addedToPhys = -1;
      //set up page in phys memory
      physMem[corrFrame].inUse = 1;
      physMem[corrFrame].page = whichPage;
      physMem[corrFrame].process = whichProc;
    }
    else{
      //all frames for the process aren't full
      while(physMem[physCounter].inUse != 0 && physCounter != pagetables[whichProc].framesPerProcess){
        //loop back to 0 when reach the end of the frames
        if(physCounter == numFrames){
          physCounter = 0;
        }
        else{
          physCounter ++;
        }
        //update the page to a position in physical memory
        pagetables[whichProc].framesInUse ++;
        pagetables[whichProc].pageFaults ++;
        pagetables[whichProc].memAccesses ++;
        pagetables[whichProc].pt[whichPage].addedToPhys = 1;
        pagetables[whichProc].pt[whichPage].frame = physCounter;
        physMem[physCounter].process = whichProc;
        physMem[physCounter].page = whichPage;
        physMem[physCounter].inUse = 1;
      }

    }
  }
  //global
  else if(evicScope == 0){
    int corrFrame = -1;
    int testProc, testPage;
    //test if all frames are full or not
    for(int j = 0; j < numFrames; j++){
      if(physMem[j].inUse == 0){
        corrFrame = j;
      }
    }
    //didn't find a frame that is free
    if(corrFrame == -1){
      //loop through and test for the page that was first in
      for(int i = 0; i < numFrames; i++){
        testProc = physMem[i].process;
        testPage = physMem[i].page;
        if(i == 0){
          swapPage = i;
          firstIn = pagetables[testProc].pt[testPage].addedToPhys;
        }
        else{
          if(pagetables[testProc].pt[i].addedToPhys < firstIn){
            swapPage = i;
            firstIn = pagetables[testProc].pt[i].addedToPhys;
          }
        }
      }
    }
    //found the first in page to pull out
    pagetables[whichProc].pageFaults ++;
    pagetables[whichProc].memAccesses ++;
    pagetables[testProc].pt[testPage].present = 0;
    pagetables[whichProc].pt[whichPage].frame = pagetables[testProc].pt[testPage].frame;
    //get the phys. frame to place
    corrFrame = pagetables[testProc].pt[testPage].frame;
    pagetables[testProc].pt[testPage].frame = -1;
    pagetables[testProc].pt[testPage].addedToPhys = -1;
    //set up page in phys memory
    physMem[corrFrame].inUse = 1;
    physMem[corrFrame].page = whichPage;
    physMem[corrFrame].process = whichProc;

  }
  else{
    //Found an empty spot in physical memory
    pagetables[whichProc].framesInUse ++;
    pagetables[whichProc].pageFaults ++;
    pagetables[whichProc].pt[whichPage].addedToPhys = counter;
    pagetables[whichProc].pt[whichPage].frame = physCounter;
    physMem[corrFrame].process = whichProc;
    physMem[corrFrame].page = whichPage;
    physMem[corrFrame].inUse = 1;

  }
}


//Second Chance eviction algorithm
void secondchance(struct pagetable * pagetables, struct frames * physMem, int whichProc, int whichPage, int evicScope, int counter, int numFrames){
  int swapPage, corrFrame;
  int firstIn = -1;
  int physCounter = 0;
  //update pagetable entry
  pagetables[whichProc].pt[whichPage].present = 1;
  //local
  if(evicScope == 1){
    //if frames for process are full
    if(pagetables[whichProc].framesPerProcess == pagetables[whichProc].framesInUse){
      //loop through all pages to find those in memory
      for(int i = 0; i < pagetables[whichProc].pagesPerProcess; i++){
        //access all pages that are currently in memory
        if(pagetables[whichProc].pt[i].present == 1){
          //check the reference bit and reset if equal to 1
          if(pagetables[whichProc].pt[i].reference == 1){
            pagetables[whichProc].pt[i].reference = 0;
          }
          else{
            swapPage = i;
            break;
          }
          //reach the end of ptable with no luck and reset back to 0  
          if(i == pagetables[whichProc].pagesPerProcess){
            i = 0;
          } 
        }
        //found a non-referenced page to pull out
        pagetables[whichProc].pt[whichPage].reference = 1;
        pagetables[whichProc].pageFaults ++;
        pagetables[whichProc].memAccesses ++;
        pagetables[whichProc].pt[swapPage].present = 0;
        pagetables[whichProc].pt[whichPage].frame = pagetables[whichProc].pt[swapPage].frame;
        //get the phys. frame to place
        corrFrame = pagetables[whichProc].pt[swapPage].frame;
        pagetables[whichProc].pt[swapPage].frame = -1;
        //set up page in phys memory
        physMem[corrFrame].inUse = 1;
        physMem[corrFrame].page = whichPage;
        physMem[corrFrame].process = whichProc;
      }
    }
    else{
      //all frames for the process aren't full
      while(physMem[physCounter].inUse != 0 && physCounter != pagetables[whichProc].framesPerProcess){
        //loop back to 0 when reach the end of the frames
        if(physCounter == numFrames){
          physCounter = 0;
        }
        else{
          physCounter ++;
        }
        //update the page to a position in physical memory
        pagetables[whichProc].framesInUse ++;
        pagetables[whichProc].pageFaults ++;
        pagetables[whichProc].memAccesses ++;
        pagetables[whichProc].pt[whichPage].addedToPhys = 1;
        pagetables[whichProc].pt[whichPage].frame = physCounter;
        physMem[physCounter].process = whichProc;
        physMem[physCounter].page = whichPage;
        physMem[physCounter].inUse = 1;
      }

    }
  }
  //global
  else if(evicScope == 0){
    int corrFrame = -1;
    int testProc, testPage;
    //test if all frames are full or not
    for(int j = 0; j < numFrames; j++){
      if(physMem[j].inUse == 0){
        corrFrame = j;
      }
    }
    //didn't find a frame that is free
    if(corrFrame == -1){
      //loop through and test for the page not referenced
      for(int i = 0; i < numFrames; i++){
        testProc = physMem[i].process;
        testPage = physMem[i].page;
        //access all pages that are currently in memory
        //check the reference bit and reset if equal to 1
        if(pagetables[testProc].pt[testPage].reference == 1){
          pagetables[testProc].pt[testPage].reference = 0;
        }
        else{
          swapPage = i;
          break;
        }
        //reach the end of phys with no luck and reset back to 0  
        if(i == numFrames){
          i = 0;
        } 
      }
      //found non-referenced page to pull out
      pagetables[whichProc].pt[whichPage].reference = 1;
      pagetables[whichProc].pageFaults ++;
      pagetables[whichProc].memAccesses ++;
      pagetables[testProc].pt[testPage].present = 0;
      pagetables[whichProc].pt[whichPage].frame = pagetables[testProc].pt[testPage].frame;
      //get the phys. frame to place
      corrFrame = pagetables[testProc].pt[testPage].frame;
      pagetables[testProc].pt[testPage].frame = -1;
      //set up page in phys memory
      physMem[corrFrame].inUse = 1;
      physMem[corrFrame].page = whichPage;
      physMem[corrFrame].process = whichProc;

    }
    else{
      //Found an empty spot in physical memory
      pagetables[whichProc].pt[whichPage].reference = 1;
      pagetables[whichProc].memAccesses ++;
      pagetables[whichProc].framesInUse ++;
      pagetables[whichProc].pageFaults ++;
      pagetables[whichProc].pt[whichPage].frame = physCounter;
      physMem[corrFrame].process = whichProc;
      physMem[corrFrame].page = whichPage;
      physMem[corrFrame].inUse = 1;

    }
  }
}

//Least Recently Used eviction algorithm
/*void LRU(struct pagetable pagetables, evicScope){

  }*/

//Least Frequently Used eviction algorithm
void LFU(struct pagetable * pagetables, struct frames * physMem, int whichProc, int whichPage, int evicScope, int counter, int numFrames){
  int swapPage, corrFrame;
  int firstIn = -1;
  int physCounter = 0;
  int lowest;
  //update pagetable entry
  pagetables[whichProc].pt[whichPage].present = 1;
  pagetables[whichProc].pt[whichPage].count ++;
  //local
  if(evicScope == 1){
    //if frames for process are full
    if(pagetables[whichProc].framesPerProcess == pagetables[whichProc].framesInUse){
      //loop through all pages to find those in memory
      for(int i = 0; i < pagetables[whichProc].pagesPerProcess; i++){
        //access all pages that are currently in memory
        if(pagetables[whichProc].pt[i].present == 1){
          //initially set lowest to first page
          if(lowest == -1){
            swapPage = i;
            lowest = pagetables[whichProc].pt[i].count;
          }
          //compare the pages as you loop
          else{
            if(pagetables[whichProc].pt[i].count < lowest){
              swapPage = i;
              lowest = pagetables[whichProc].pt[i].count;
            }
          }
        }
      }
      //found the lowest page to pull out
      pagetables[whichProc].pageFaults ++;
      pagetables[whichProc].memAccesses ++;
      pagetables[whichProc].pt[swapPage].present = 0;
      pagetables[whichProc].pt[whichPage].frame = pagetables[whichProc].pt[swapPage].frame;
      //get the phys. frame to place
      corrFrame = pagetables[whichProc].pt[swapPage].frame;
      pagetables[whichProc].pt[swapPage].frame = -1;
      pagetables[whichProc].pt[swapPage].addedToPhys = -1;
      //set up page in phys memory
      physMem[corrFrame].inUse = 1;
      physMem[corrFrame].page = whichPage;
      physMem[corrFrame].process = whichProc;
    }
    else{
      //all frames for the process aren't full
      while(physMem[physCounter].inUse != 0 && physCounter != pagetables[whichProc].framesPerProcess){
        //loop back to 0 when reach the end of the frames
        if(physCounter == numFrames){
          physCounter = 0;
        }
        else{
          physCounter ++;
        }
        //update the page to a position in physical memory
        pagetables[whichProc].framesInUse ++;
        pagetables[whichProc].pageFaults ++;
        pagetables[whichProc].memAccesses ++;
        pagetables[whichProc].pt[whichPage].frame = physCounter;
        physMem[physCounter].process = whichProc;
        physMem[physCounter].page = whichPage;
        physMem[physCounter].inUse = 1;
      }

    }
  }
  //global
  else if(evicScope == 0){
    int corrFrame = -1;
    int testProc, testPage;
    //test if all frames are full or not
    for(int j = 0; j < numFrames; j++){
      if(physMem[j].inUse == 0){
        corrFrame = j;
      }
    }
    //didn't find a frame that is free
    if(corrFrame == -1){
      //loop through and test for the page that has lowest count
      for(int i = 0; i < numFrames; i++){
        testProc = physMem[i].process;
        testPage = physMem[i].page;
        if(pagetables[testProc].pt[testPage].present == 1){
          if(i == 0){
            swapPage = i;
            lowest = pagetables[testProc].pt[testPage].count;
          }
          else{
            if(pagetables[testProc].pt[i].count < lowest){
              swapPage = i;
              lowest = pagetables[testProc].pt[i].count;
            }
          }
        }
      }
      //found the lowest page to pull out
      pagetables[whichProc].pageFaults ++;
      pagetables[whichProc].memAccesses ++;
      pagetables[testProc].pt[testPage].present = 0;
      pagetables[whichProc].pt[whichPage].frame = pagetables[testProc].pt[testPage].frame;
      //get the phys. frame to place
      corrFrame = pagetables[testProc].pt[testPage].frame;
      pagetables[testProc].pt[testPage].frame = -1;
      //set up page in phys memory
      physMem[corrFrame].inUse = 1;
      physMem[corrFrame].page = whichPage;
      physMem[corrFrame].process = whichProc;

    }
    else{
      //Found an empty spot in physical memory
      pagetables[whichProc].framesInUse ++;
      pagetables[whichProc].pageFaults ++;
      pagetables[whichProc].pt[whichPage].frame = physCounter;
      physMem[corrFrame].process = whichProc;
      physMem[corrFrame].page = whichPage;
      physMem[corrFrame].inUse = 1;

    }
  }
}



int main(int argc, char **argv) {
  int whichPage, virtAdd, whichProc, opt, numPages, numBytes, framesPer, numprocesses, mainSize, pgSize, allocAlg, evicAlg, evicScope, period, numFrames;
  FILE *plistPtr;
  FILE *ptracePtr;
  FILE *ptab;
  int globFirst = 0;
  int tracker = 0;
  int framesUsed = 0;
  int frameCounter = 0;
  int counter = 0;
  int combSize = 0;
  int ptraceCounter = 1;
  char * line = NULL;
  ssize_t length;
  size_t n;

  if(argc != 7){
    perror("argument error: ");
    exit(1);
  }

  //allow stdout to print regardless of a buffer
  setbuf(stdout, NULL);

  //Intake of command line arguments
  while((opt = getopt(argc, argv, "m:s:a:e:f:p:")) != -1){
    switch(opt){
      case 'm':
        //size of main memory in bytes
        mainSize = atoi(optarg);
        break;
      case 's':
        //page size in bytes
        pgSize = atoi(optarg);
        break;
      case 'a':
        //equal(0) or proportional(1)
        allocAlg = atoi(optarg);
        break;
      case 'e':
        //eviction algo: FIFO(0), Second Chance(1), LRU(2), LFU(3)
        evicAlg = atoi(optarg);
        break;
      case 'f':
        //eviction scope: global(0) or local(1)
        evicScope = atoi(optarg);
        break;
      case 'p':
        //period: num. mem. accesses before printing page tables
        period = atoi(optarg);
        break;
      default:
        printf("Ya messed up your parameters \n");
        exit(1);
    }
  }
  //num frames available based on sizing
  numFrames = mainSize / pgSize;

  struct pages globalq[numFrames];

  //open up plist.txt
  int plist = open("plist.txt", O_RDONLY);
  if(plist == -1){
    perror("open copy: ");
    exit(1);
  }
  //plist FILE *
  plistPtr = fdopen(plist, "r");

  //open up the trace
  int ptrace = open("ptrace.txt", O_RDONLY);
  if(ptrace == 1){
    perror("open ptrace:");
    exit(1);
  }
  //ptrace FILE *
  ptracePtr = fdopen(ptrace, "r");


  //See if path to pasteFile exists, and delete if so 
  if(access("ptable.txt", F_OK) == 0){
    if(remove("ptable.txt") == -1){
      printf("Could not delete second file");
      exit(1);
    }
  }

  //Create and open pasteFile for reading and writing
  int ptable = open("ptable.txt", O_CREAT|O_RDWR, S_IRWXU);
  if(ptable == -1 || ptable < 0){
    perror("open paste: ");
    exit(1);
  }
  //FILE * to write to ptable.txt
  ptab = fdopen(ptable, "w+");

  //get number of processes from first line
  length = getline(&line, &n, plistPtr);
  if (length == -1){
    perror("initial getline() ");
    exit(1);
  }
  numprocesses = atoi(line);

  //array of all pagetables
  struct pagetable pagetables[numprocesses];

  //array for frames in phys. memory
  struct frames physMem[numFrames];

  //initialize each to unused
  for(int q = 0; q < numFrames; q++){
    physMem[q].inUse = 0;
  }

  //accumulate data on the processes
  while((length = getline(&line, &n, plistPtr)) != -1){
    //split line into words to get process size
    strtok(line, " ");
    numBytes = atoi(strtok(NULL, " "));
    pagetables[counter].procSize = numBytes;
    //accumulate size of all processes together for proportional allocation
    combSize += numBytes;
    //how many pages in page table is num bytes of process divided by pg size
    numPages = numBytes / pgSize;
    if(numBytes % pgSize != 0){
      numPages += pgSize;
    }
    pagetables[counter].pagesPerProcess = numPages;
    struct pages spec[numPages];
    pagetables[counter].pt = spec;
    counter ++;
  }

  //initialize framesUsed,present, count, and time to 0 for every page in each process
  for(int k = 0; k < numprocesses; k++){
    pagetables[k].framesInUse = 0;
    pagetables[k].memAccesses = 0;
    for(int l =0; l < pagetables[k].pagesPerProcess; l++){
      pagetables[k].pt[l].present = 0;
      pagetables[k].pt[l].count = 0;
      pagetables[k].pt[l].time = 0;
      pagetables[k].pt[l].addedToPhys = -1;
    }
  }

  //divide up frames equally
  if (allocAlg == 0){
    framesPer = numFrames / numprocesses;
    for(int i = 0; i < numprocesses; i++){
      pagetables[i].framesPerProcess = framesPer;
    }
  }

  //divide up frames proportionally
  else{
    for(int j = 0; j < numprocesses; j++){
      pagetables[j].pageFaults = 0;
      int percentage = combSize / pagetables[j].procSize;
      framesPer = numFrames / (percentage / 100);
      pagetables[j].framesPerProcess = framesPer;
    }
  }


  while((length = getline(&line, &n, ptracePtr)) != -1){
    if(period != 0){
      if((ptraceCounter % period) == 0){
        //write to file the page tables
        fprintf(ptab, "----------------------- Time: %d ----------------------\n", period);
        for(int s = 0; s < numprocesses; s++){
          fprintf(ptab, "Process %d:\n", s);
          for(int t = 0; t < pagetables[s].pagesPerProcess; t++){
            fprintf(ptab, "page:%d present:%d reference:%d frame:%d count:%d time:%d\n", 
              t, pagetables[s].pt[t].present, pagetables[s].pt[t].reference, pagetables[s].pt[t].frame,
              pagetables[s].pt[t].count, pagetables[s].pt[t].time);
          }
        }
      }
    }
    whichProc = atoi(strtok(line, " "));
    virtAdd = atoi(strtok(NULL, " "));
    //determine which page to check
    whichPage = virtAdd / pgSize;
    //check if page is already present and update counter 
    if(pagetables[whichProc].pt[whichPage].present == 1){
      pagetables[whichProc].pt[whichPage].count ++;
      pagetables[whichProc].memAccesses ++;
      pagetables[whichProc].pt[whichPage].reference = 1;
      pagetables[whichProc].pt[whichPage].count ++;
      pagetables[whichProc].pt[whichPage].time = ptraceCounter;
    }
    //page is not present in main memory
    else{       
      if(evicAlg == 0){
        fifo(pagetables, physMem, whichProc, whichPage, evicScope, ptraceCounter, numFrames);
      }
      else if(evicAlg == 1){
        secondchance(pagetables, physMem, whichProc, whichPage, evicScope, ptraceCounter, numFrames);
      }
      else if(evicAlg == 2){
        //LRU(pagetables, evicScope);
      }
      else{
        LFU(pagetables, physMem, whichProc, whichPage, evicScope, ptraceCounter, numFrames);
      }
    }

    //increment time counter after each getline each loop
    for(int o = 0; o < numprocesses; o++){
      for(int p = 0; p < pagetables[o].pagesPerProcess; p++){
        if(pagetables[o].pt[p].present == 1){
          pagetables[o].pt[p].time ++;
        }
      }
    }

    //place frame counter back at top when reached the end
    if(frameCounter == (numFrames - 1)){
      frameCounter = 0;
    }
    //counting how many ptrace events
    ptraceCounter ++;
  }
  //print results
  float totalPFaults;
  float totalMemAccesses;
  float pf;
  float ma;
  float locDiv;
  float bigDiv;
  for(int y = 0; y < numprocesses; y ++){
    printf("Process %d faults: %d/%d\n", y, pagetables[y].pageFaults, pagetables[y].memAccesses);
    pf = pagetables[y].pageFaults;
    ma = pagetables[y].memAccesses;
    locDiv = (pf/ma);
    printf("Process %d fault percentage: %f\n\n", y, locDiv);
    totalPFaults += pagetables[y].pageFaults;
    totalMemAccesses += pagetables[y].memAccesses;
  }
  printf("Total faults: %f/%f\n", totalPFaults, totalMemAccesses);
  bigDiv = totalPFaults/totalMemAccesses;
  printf("Total fault percentage: %f\n", bigDiv);
}
