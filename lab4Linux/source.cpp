#include <iostream>
#include <pthread.h>
#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>

using namespace std;

pthread_mutex_t mutex;

int getch() {
struct termios oldt,
newt;
int ch;
tcgetattr( STDIN_FILENO, &oldt );
newt = oldt;
newt.c_lflag &= ~( ICANON | ECHO );
tcsetattr( STDIN_FILENO, TCSANOW, &newt );
ch = getchar();
tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
return ch;
}

void* print (void* ptr){

    int num = *(int*)ptr;                                                          
    while(1){

        pthread_mutex_lock(&mutex);                                                
        cout << "Thread №" << num << " says Hello!" << endl;
        pthread_mutex_unlock(&mutex);                                              
        sleep(1);       
    }
}

int main(){

    int f = 1;
    char a;
    int num = 0;

    pthread_mutex_init(&mutex, NULL);                                              

    pthread_t newThread;                                                                  
    vector<pthread_t> threads;                                                     

    system("clear");

    while(f){

        a=getch();                                                        
 
        switch(a){

            case('+'):

                pthread_create(&newThread, NULL, print, &num);                    
                threads.push_back(newThread);
                num++;
                break;

            case('-'):

                if(!threads.size()) break;
                pthread_cancel(threads[threads.size() - 1]);                       
                threads.pop_back();
                break;

            case('q'):
                f=0;                                                               
                break;
        }
        a = 0;
    }   

    while(threads.size()){

        pthread_cancel(threads[threads.size() - 1]);
        threads.pop_back();
    }

    pthread_mutex_destroy(&mutex);                                                 

    return 0;
}
