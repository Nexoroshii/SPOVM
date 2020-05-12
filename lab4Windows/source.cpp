#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include <stack>
#include <stdlib.h>
#include <stdio.h>
#include <stack>
#include <iostream>

using namespace std;

DWORD WINAPI printID(PVOID mutex)
{
	char buf[10];
	int threadID = (int)GetCurrentThreadId();
	_itoa(threadID, buf, 10);
	while (1)
	{
		if (WaitForSingleObject(mutex, INFINITE) == WAIT_OBJECT_0)
		{
			for (int i = 0; buf[i] != '\0'; i++)
			{
				cout << buf[i];
				Sleep(350);
			}
			cout << endl;
			ReleaseMutex(mutex);
		}
	}
	return 0;
}

void deleteOneThread(HANDLE thread)
{
	TerminateThread(thread, 0);
}

HANDLE createNewThread(HANDLE &mutex)
{
	HANDLE thread;
	thread = CreateThread(NULL,								//Атрибут безопастности
		0,									//Размер стека,выделяемый под поток
		(LPTHREAD_START_ROUTINE)printID,	//Адрес потоковой функции
		mutex,								//Указатель на параметры функции
		0,									//Флаг создания потока
		NULL);								//Указатель, в который записывается идентификатор данного потока
	return thread;
}

void init(HANDLE *mutex)
{
	*mutex = CreateMutex(NULL,								//Атрибут безопастности
		FALSE,								//Флаг начального владельца
		NULL);								//Имя объекта
}

void deleteAll(stack <HANDLE> &stack)
{
	while (stack.size()) {
		deleteOneThread(stack.top());
		stack.pop();
	}
}

int main()
{
	stack <HANDLE> threads;

	HANDLE mutex;
	init(&mutex);


	cout << "PRESS:" << endl << "+ for add new thread\n" << "- for delete last thread\n" << "q to exit\n" << endl;
	while (1)
	{
		switch (_getch())
		{
		case '=':
		{
			threads.push(createNewThread(mutex));
		}
		break;
		case '-':
		{
			if (threads.size())
			{
				WaitForSingleObject(mutex, INFINITE);
				deleteOneThread(threads.top());
				threads.pop();
				ReleaseMutex(mutex);
			}
		}
		break;
		case 'q':
		{
			WaitForSingleObject(mutex, INFINITE);
			deleteAll(threads);
			return 0;
		}
		break;
		}
	}

	return 0;
}


