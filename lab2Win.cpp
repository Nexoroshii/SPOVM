#define _CRT_SECURE_NO_WARNINGS
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <Windows.h>
#include <list>
#include <conio.h>

using namespace std;

const int DELAY = 40;
const int BUFF_SIZE = 10;
const int MAIN_DELAY = 500;
const int MAIN_PROCESS_ARGS = 1;

HANDLE createNewProcess(string source, int procNumber);
void printProcessSignature(string procNumber);

int main(int argc, char** argv) {

	if (argc > MAIN_PROCESS_ARGS) {	// if it is child

		HANDLE eventHdl = OpenEvent(EVENT_ALL_ACCESS,	// флаги доступа						EVENT_ALL_ACCESS - указывает все возможные флаги доступа
			false,				// флаги наследования 
			"mainEvent");		// адрес имени объекта-события

		string information = argv[1];

		while (true) {
			WaitForSingleObject(eventHdl, INFINITE);
			printProcessSignature(information);
			SetEvent(eventHdl);
		}
	}
	else {

		HANDLE mainEvent = CreateEvent(NULL,			// атрибуты защиты
			FALSE,			// флаг ручного сброса события (TRUE - вручную, FALSE - автоматичкски)
			true,			// флаг начального состояния события (TRUE - сигнальное состояние)
			"mainEvent");	// адрес имении объекта-события

		list <HANDLE> processList;

		while (true) {
			WaitForSingleObject(mainEvent, INFINITE);
			cout << "*************" << endl;

			if (_kbhit())
				switch (_getch()) {
				case '+':
					processList.push_front(createNewProcess(string(argv[0]), processList.size() + 1));
					break;
				case '-':
					if (!processList.empty()) {
						TerminateProcess(processList.front(), EXIT_SUCCESS);
						CloseHandle(processList.front());
						processList.pop_front();
					}
					else {
						cout << "List is empty, create process please" << endl;
					}
					break;
				case 'q':
					while (!processList.empty()) {
						TerminateProcess(processList.front(), EXIT_SUCCESS);
						CloseHandle(processList.front());
						processList.pop_front();
					}

					CloseHandle(mainEvent); // закрываем описатели процесса и потока
					return 0;
				}

			SetEvent(mainEvent);		// установка события в отмеченное состояние
			Sleep(MAIN_DELAY);
		}
	}
}

HANDLE createNewProcess(string source, int procNumber) {

	char commandArgs[BUFF_SIZE];
	sprintf(commandArgs + 1, "%d", procNumber);
	commandArgs[0] = ' ';

	STARTUPINFO sturtupInfo;
	PROCESS_INFORMATION processInfo;

	memset(&sturtupInfo, 0, sizeof(sturtupInfo));
	memset(&processInfo, 0, sizeof(processInfo));

	sturtupInfo.cb = sizeof(sturtupInfo);

	if (!CreateProcess(source.c_str(),					// Имя модуля
		commandArgs,									// Командная строка
		NULL,											// Дескриптор процесса
		NULL,											// Дескриптор потока
		FALSE,											// Установка описателей наследования
		NULL, //CREATE_NEW_CONSOLE,						// Флаг создания процесса
		NULL,											// блок переменных окружения родительского процесса
		NULL,											// Текущий каталог родительскоого процесса
		&sturtupInfo,									// указатель на  структуру
		&processInfo)									// Указатель на структуру информаций о процессе
		) {
		cout << "Error while creating new process!" << endl;
		Sleep(MAIN_DELAY);
		exit(EXIT_FAILURE);
	}

	return processInfo.hProcess;
}

void printProcessSignature(string procNumber) {
	for (int i = 0; i < procNumber.length(); i++) {
		cout << procNumber[i];
		Sleep(DELAY);
	}

	string message = " Process   *";
	for (int i = 0; i < message.length(); i++) {
		cout << message[i];
		Sleep(DELAY);
	}
	cout << endl;

}
