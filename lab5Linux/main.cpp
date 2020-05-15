#include <iostream>
#include <dlfcn.h>

using namespace std;

int main(){

    system("clear");

    void* lib;                                                              //Äåñêðèïòîð äèíàìè÷åñêîé áèáëèîòåêè
    void (*concat)(void);                                                   //Óêàçàòåëü íà ôóíêöèþ

    lib = dlopen("/home/nick/spo/Lib/libconcatFiles.so", RTLD_LAZY);        //Çàãðóçêà áèáëèîòåêè â ïàìÿòü

    *(void **) (&concat) = dlsym(lib, "concat");                            //Ïîëó÷åíèå àäðåñà ôóíêöèè

    concat();                                                               //Âûçîâ ôóíêöèè

    dlclose(lib);                                                           //Âûãðóçêà áèáëèîòåêè â ïàìÿòè

    return 0;
}
