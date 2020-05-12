#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
 
#define MAX_COUNT 3
 
char outString[3][15] ={ {"|||||||"}, {"*******"}, {"#######"}, };
pid_t child_pid[MAX_COUNT+1];           //stack with all child processes
 
int fend = 1;
int cur = 1;
int size = 0;
int fprint = 0;
int fquit = 0;
//obrabotchiki signalov 
struct sigaction printSignal, endSignal;

//zdes' ispolzujutsa SUGUSR1 and SIGUSR2 eto ne est' horosho
 
void checkEnd()
{
    if(fend)
    {
          kill(getppid(),SIGUSR2); //signal to parent about finish of printing
          exit(0);
    }
}
 
 //signo - number of signal
void canPrint(int signo)
{
  fprint = 1;
}
 
void setEndFlag(int signo)
{
  fend = 1;
}
 
int main(void)
{
      initscr();
      clear();
      noecho();
      refresh();
 
      printSignal.sa_handler = canPrint;
      //sa_handler choose the signal
      //canPrint = 1 for output
      //setEnd = 1 for kill
      sigaction(SIGUSR1,        //func-obr
                &printSignal,   //mask
                NULL); 
 
      endSignal.sa_handler = setEndFlag;
      sigaction(SIGUSR2,
                &endSignal,
                NULL);
 
      char input = 0;
 
      child_pid[0]=getpid();
 
      while(input!='q')
      {

        switch(input=getchar())
        {
            case '=':
            {
                //check 
                if(size <= MAX_COUNT)
                {
                    size++;
                    child_pid[size] = fork();
                }

                switch(child_pid[size])
                {
                    case 0: //child process
                    {
                        fend = 0;
                        while(!fend)
                        {
                            //if fprint in some process == 1
                            if(fprint)
                            {
                                for(int i=0; i<strlen(outString[size-1]); i++)
                                {
                                    checkEnd();//when we push '-' we need to synchronize processes
                                    printf(" \r\n%c ",outString[size-1][i]);
                                    usleep(40000);
                                    refresh();
                                }
                                checkEnd();
                                refresh();
                                fprint = 0;
                                kill(getppid(),SIGUSR2); //signal to parent about finish of printing
                            }
                            checkEnd();
        
                        }

                        return 0;

                    } break;

                    case -1:
                    {
                        printf("Process [%d] failed!\n",size);
                    }
                    break;
                }
            } break;

            case '-':
            {
                if(size==0)
                    break;
                kill(child_pid[size],SIGUSR2); //finishing last child process
                waitpid(child_pid[size],//id of process wich we waiting
                                NULL,//without status
                                0);//without options
                if(cur == size) cur = 1;
                size--;
            } break;
        }
          
          if(fend && size>0)       
          {
              fend = 0;
              kill(child_pid[cur++],SIGUSR1); 
              if(cur > size)   
                  cur = 1;                  
          }
          refresh();
      }
 
      //finishing all child processes
      if(child_pid[size]!=0)
      {
         for(;size>0;size--)
         {
             kill(child_pid[size],SIGUSR2);
             waitpid(child_pid[size],NULL,0);
         }
      }
      clear();
      endwin();
 
      return 0;
}
