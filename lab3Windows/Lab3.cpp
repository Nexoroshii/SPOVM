#define _CRT_SECURE_NO_WARNINGS

#include <conio.h>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <limits>

using namespace std;

const int MaxSize = 20;
const int Delay = 100;
const DWORD MaximumSizeLow = 0;
const DWORD MaximumSizeHigh = MaxSize;

void Child()
{
	HANDLE Work = OpenSemaphore(SEMAPHORE_ALL_ACCESS,	//доступ к объекту семафора
		FALSE,					//процессы не наследуют этот дексриптор
		"Work");
	HANDLE Close = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "Close");
	HANDLE WINAPI FileProjection = OpenFileMapping(FILE_MAP_ALL_ACCESS,	//доступ к объекту сопоставления файлов
		FALSE,				//не наследует дескриптор
		"FileProjection");	//имя объекта сопоставления файлов

	LPVOID memoryMap;
	memoryMap = memoryMap = MapViewOfFile(FileProjection, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	char* outputString = (char*)memoryMap;

	while (true)
	{
		//ожидает вызова у предка ReleaseSemaphore
		WaitForSingleObject(Work, INFINITE);
		//когда дождались увелечения счётчика семафора закрытия,то переходим к закрытию всего
		if (WaitForSingleObject(Close, Delay) == WAIT_OBJECT_0)
		{
			//закрываем дескриптор объектов:всех семафоров,объекта сопоставления файлов 
			CloseHandle(Work);
			CloseHandle(Close);
			UnmapViewOfFile(memoryMap);
			CloseHandle(FileProjection);
			return;
		}

		cout << "Client: " << outputString << endl;
		//увеличиваем счётчик семафора на 1
		ReleaseSemaphore(Work, 1, NULL);
	}
}

void Parent(char* input)
{
	HANDLE Work = CreateSemaphore(NULL,		//Дексриптор безопасности по умолчанию
		0,		//Начальный счёт для объекта семафора
		1,		//Максимальное состояние счётчика
		"Work");	//Если lpName соответствует имени существующего объекта семафора, эта функция запрашивает право доступа SEMAPHORE_ALL_ACCESS

	HANDLE Close = CreateSemaphore(NULL,
		0,
		1,
		"Close");

	HANDLE WINAPI FileProjection = CreateFileMapping(INVALID_HANDLE_VALUE,	//объект сопоставления фаойлов указанного размера(в качестве проецируемого файла будет исп файл подкачки)
		NULL,					//декриптор не унаследуется
		PAGE_READWRITE,			//Защита страницы объекта сопоставления файлов
		MaximumSizeLow,			//минимальный размер
		MaximumSizeHigh,		//максимальный размер
		"FileProjection");		//омя объекта сопоставления файлов

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi;

	if (!CreateProcess(input,				//имя
		(LPSTR)"child process",		//командная строка
		NULL,				//атрибуты процесса
		NULL,				//атрибуты потока
		FALSE,				//наследование
		CREATE_NEW_CONSOLE,
		NULL,				//среда
		NULL,				//директория
		&si,
		&pi))
	{
		cout << "Create Process failed" << GetLastError() << endl;
		system("pause");
		exit(EXIT_FAILURE);
	}

	//получаем адрес в памяти,чтобы записать туда передаваемые данные
	LPVOID memoryMap;
	memoryMap = MapViewOfFile(FileProjection,		//десриптор файловой проекции,полученный от предыдущей функции
		FILE_MAP_ALL_ACCESS,	//режим доступа к области памяти	
		0,						//старшее слово смещения файла, с которого начинается отображение
		0,						//малдщее слово смещения
		0);						//число отображаемых байт(если 0,то отображается весь файл)

	char* inputString = (char*)memoryMap;

	int curPos = 0;
	bool readyForInput = true;
	string buf;
	buf.resize(MaxSize, '\0');

	while (true)
	{
		if (readyForInput)
		{
			curPos = 0;
			cout << "Server: ";
			getline(cin, buf);
			readyForInput = false;
		}
		cout << endl;
		string tmp;
		int lenght = 0;
		tmp.append(buf, 0, MaxSize - 1);		//добавить символ в конец строки
		curPos = tmp.length();

		strcpy(inputString, const_cast<char*>(tmp.c_str()));
		inputString[tmp.length()] = '\0';

		tmp.clear();
		lenght = buf.length() - curPos;
		if (lenght > 0)
		{
			tmp.append(buf, curPos, lenght);
		}

		buf = tmp;

		//Для вывода в CLIENT 
		ReleaseSemaphore(Work,		//название семафора
			1,			//изменение счётчика
			NULL);		//пердыдущее значение
		WaitForSingleObject(Work, INFINITE);

		if (buf.empty())
		{
			readyForInput = true;
			if (cin.get() == 'e')	//для выхода
			{
				ReleaseSemaphore(Close, 1, NULL);
				ReleaseSemaphore(Work, 1, NULL);
				WaitForSingleObject(pi.hProcess, INFINITE);
				UnmapViewOfFile(memoryMap);					//Отключает сопоставленный вид файла из адресного пространства вызывающего процесса.
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				CloseHandle(Close);
				CloseHandle(Work);
				CloseHandle(FileProjection);
				return;
			}
			buf.clear();
			cin.clear();
		}
	}
}

void main(int argc, char* argv[]) {
	if (argc > 1) {
		Child();
	}
	else {
		Parent(argv[0]);
	}
}
