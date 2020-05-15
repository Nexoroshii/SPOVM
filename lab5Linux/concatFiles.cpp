#include <iostream>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <aio.h>
#include <string.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 80

using namespace std;

// .txt в конце строки
bool isTxt(char* str){

    int i = 0;
    char search[] = "txt.";

    while(str[i+1]) i++;

    for(int j = 0; j < 4; j++)
        if(str[i-j] != search[j]) return 0;

    return 1;
}

bool isValid(char* str){

    if(!isTxt(str))
        return 0;

    if(!strcmp(str, "output.txt"))
        return 0;

    return  1;
}

//Поток-писатель
void* writer (void* ptr){

    int pipe = *(int*)ptr;                                                     //Дескриптор чтения из канала

    char buf[BUF_SIZE];                                                        //Буфер для чтения
    ssize_t symRead;                                                           //Размер строки, прочитанной из канала
  
    int sem = semget(ftok("./main.cpp", 1), 0, IPC_CREAT | 0666);
    struct sembuf wait = {0, 0, SEM_UNDO};                                     //Ожидание семафора чтения
    struct sembuf lock = {0, 1, SEM_UNDO};                                     //Блокировка семафора чтения
    struct sembuf unlock = {1, -1, SEM_UNDO};                                  //Разблокировка семафора записи
    semop(sem, &lock, 1);

    int file;                                                                  //Дескриптор файла
    file = open("output.txt", O_WRONLY | O_TRUNC | O_CREAT);                   //Открытие выходного файла

    aiocb writeFile;                                                           //Структура для асинхронной записи
    writeFile.aio_fildes = file;                                               //Файл в который производится запись
    writeFile.aio_offset = 0;                                                  //Отступ в файле
    writeFile.aio_buf = &buf;                                                  //Записываемая строка


    while(1){

        semop(sem, &wait, 1);                                                  //Ожидание разрешения чтения из канала
        semop(sem, &lock, 1);

        symRead = read(pipe, buf, BUF_SIZE);                                   //Чтение из канала в буфер

        if(!symRead) break;

        writeFile.aio_nbytes = symRead;                                        //Кол-во символов для записи
        aio_write(&writeFile);                                                 //Асинхронная запись

        while(aio_error(&writeFile) == EINPROGRESS);                           //Ожидание окончания записи

        writeFile.aio_offset += symRead;                                       //Сдвиг отступа

        semop(sem, &unlock, 1);                                                //Разрешение записи в канал
    }

}

//Поток-читатель
void* reader (void* ptr){

    int pipe = *(int*)ptr;                                                     //Дескриптор записи в канал

    char buf[BUF_SIZE];                                                        //Строка-буфер

    int sem = semget(ftok("./main.cpp", 1), 2, IPC_CREAT | 0666);              // 0 - семафор чтения, 1 - семафор записи
    struct sembuf wait = {1, 0, SEM_UNDO};                                     //Ожидание семафора записи
    struct sembuf lock = {1, 1, SEM_UNDO};                                     //Блокировка семафора записи
    struct sembuf unlock = {0, -1, SEM_UNDO};                                  //Разблокировка семафора чтения

    aiocb readFile;                                                            //Структура для асинхронного чтения
    readFile.aio_buf = &buf;                                                   //Буфер для чтения

    DIR* directory;                                                            //Рабочая директория
    dirent* nextFile;                                                          //Объект в директории
    directory = opendir("/home/nick/spo/Fifth");                               //Открытие директории

    int file;                                                                  //Дескриптор файла
    struct stat stat;                                                          //Характеристики файла
    int size;                                                                  //Размер файла

    while(1){

        nextFile = readdir(directory);                                         //Следующий объект в директории

        if(nextFile == NULL) break;                                            //Директория пуста

        if(!isValid(nextFile->d_name))                                         //Валидация обекта ( txt файл, != output.txt )
            continue;

        file = open(nextFile->d_name, O_RDONLY);                               //Открытие файла

        fstat(file, &stat);                                                    //Получение характеристик файла
        size = stat.st_size;                                                   //размер файла

        readFile.aio_fildes = file;                                            //Уст. файла для асинхронного чтения
        readFile.aio_offset = 0;                                               //Отступ в файле


        while(1){

            //Размер считываемой строки
            if(size > BUF_SIZE) readFile.aio_nbytes = BUF_SIZE;
            else readFile.aio_nbytes = size;

            aio_read(&readFile);                                               //Асинхронное чтение

            while(aio_error(&readFile) == EINPROGRESS);                        //Ожидание окончания чтения

            semop(sem, &wait, 1);                                              //Ожидание разрешения записи в канал
            semop(sem, &lock, 1);
           
            write(pipe, buf, readFile.aio_nbytes);                             //Запись строки в канал

            semop(sem, &unlock, 1);                                            //Разрешение чтения из канала

            //Изменения оставшегося размера и отступа
            if(size > BUF_SIZE){
                size -= BUF_SIZE;
                readFile.aio_offset += BUF_SIZE;
            }
            else   break;   
        }

        close(file);                                                           //Закрытие файла
    }

    semop(sem, &wait, 1);                                                      //Ожидание окончания чтения из канала
}

//Внешняя функция 
extern "C" void concat(){

    int hndl[2];                                                               //Дескрипторы канала

    //Открытие канала
    if(pipe(hndl)){
        cout << "Failed to create pipe!" << endl;
        return;
    }

    //Создание потока-писателя
    pthread_t writer_thread;         
    if(pthread_create(&writer_thread, NULL, writer, &hndl[0])){

        cout << "Failed to create writer_thread" << endl;
        return;
    }   

    //Создание потока-читателя
    pthread_t reader_thread;         
    if(pthread_create(&reader_thread, NULL, reader, &hndl[1])){

       cout << "Failed to create reader_Thread" << endl;
       pthread_cancel(writer_thread);
       return;
    }

    pthread_join(reader_thread, NULL);                                         //Ожидание окончания работы потока-читателя
    pthread_cancel(writer_thread);                                             //Завершение работы потока-писателя

    return;
}
