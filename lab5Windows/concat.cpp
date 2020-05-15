#include "pch.h"
#include "concat.h"

using namespace std;

DWORD WINAPI reader(LPVOID lpParam) {

	HANDLE hWrite = *(HANDLE*)lpParam;													//Äåñêðèïòîð äëÿ çàïèñè â êàíàë

	HANDLE hFind;																		//Äåñêðèïòîð ïîèñêà ôàéëà
	HANDLE hFile;																		//Äåñêðèïòîð ôàéëà
	WIN32_FIND_DATA FindFileData;

	int read;																			//Êîëè÷åñòâî ïðî÷èòàííûõ ñèìâîëîâ
	const int bufSize = 100;															//Ðàçìåð áóôåðà
	char buf[bufSize];																	//Áóôåð
	char separator = '\n';																//Ðàçäåëèòåëü ìåæäó ôàéëàìè


	//Ñîçäàíèå ñèãíàëà îá îêîí÷àíèè ðàáîòû
	HANDLE finished;
	finished = CreateEvent(NULL, TRUE, FALSE, "finished");
	if (!finished) {
		cout << "Failed to create finished Event!" << endl;
		return 0;
	}

	//Ñîçäàíèå ñèíõðîíèçèðóþøåãî ñèãíàëà
	HANDLE readyToRead;
	readyToRead = CreateEvent(NULL, TRUE, FALSE, "readyToRead");
	if (!readyToRead) {
		cout << "Failed to create readyToRead Event!" << endl;
		return 0;
	}

	//Overlapped äëÿ ÷òåíèÿ èç ôàéëà
	HANDLE fileReader;
	fileReader = CreateEvent(NULL, TRUE, FALSE, "fileReader");
	if (!fileReader) {
		cout << "Failed to create fileReader Event!" << endl;
		return 0;
	}

	OVERLAPPED file = { 0 };
	file.hEvent = fileReader;

	//Overlapped äëÿ çàïèñè â êàíàë
	HANDLE pipeWriter;
	pipeWriter = CreateEvent(NULL, TRUE, FALSE, "pipeWriter");
	if (!pipeWriter) {
		cout << "Failed to create pipeWriter Event!" << endl;
		return 0;
	}

	OVERLAPPED pipe = { 0 };
	pipe.hEvent = pipeWriter;

	char searchDir[MAX_PATH];															//Ïóòü äèðåêòîðèè, ñîäåðæàùåé èñõîäíûå ôàéëû
	GetModuleFileName(NULL, searchDir, MAX_PATH);										//Ïîëó÷åíèå ïóòè çàïóùåííîãî exe

	int i = MAX_PATH - 1;
	int j = 0;

	//Ïðåîáðàçîâàíèå ïóòè exe ê ñòðîêå ïîèñêà txt ôàéëîâ â äàííîé äèðåêòîðèè
	while (i) {

		if (searchDir[i - 1] == '\\') {

			if (j) {
				char searchAttr[] = "*.txt";
				for (j = 0; j < 5; j++)
					searchDir[i + j] = searchAttr[j];

				searchDir[i + 5] = 0;
				break;
			}
			else j++;
		}
		i--;
	}

	Sleep(100);

	//Îòêðûòèå ñèíõðî ñèãíàëà
	HANDLE readyToWrite = OpenEvent(EVENT_ALL_ACCESS, FALSE, "readyToWrite");
	if (!readyToWrite) {
		cout << "Failed to open readyToWrite Event!" << endl;
		return 0;
	}

	if ((hFind = FindFirstFile(searchDir, &FindFileData)) != INVALID_HANDLE_VALUE) {	//Ïîèñê txt ôàéëîâ â çàä. äèðåêòîðèè

		do {

			if (!strcmp(FindFileData.cFileName, "output.txt")) continue;				//Ïðîïóñê ôàéëà äëÿ âûâîäà

			//Îòêðûòèå ôàéëà
			hFile = CreateFile(FindFileData.cFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
			if (!hFile) {
				cout << "Failed to open a file!" << endl;
				return 0;
			}

			file.Offset = 0;															//Óñòàíîâêà íóëåâîãî ñäâèãà â OVERLAPPED ôàéëà

			do {

				ReadFile(hFile, &buf, bufSize, NULL, &file);							//×òåíèå ñîîáùåíèÿ èç ôàéëà â áóôåð
				WaitForSingleObject(fileReader, INFINITE);								//Îæèäàíèå îêîí÷àíèÿ ÷òåíèÿ

				read = file.InternalHigh;												//Êîë-âî ïðî÷èòàííûõ ñèìâîëîâ
				file.Offset += read;													//Ñäâèã îòñòóïà â ôàéëå

				WaitForSingleObject(readyToWrite, INFINITE);							//Îæèäàíèå ðàçðåøåíèÿ íà çàïèñü â êàíàë
				ResetEvent(readyToWrite);

				if (!read) {

					WriteFile(hWrite, &separator, 1, NULL, &pipe);						//Çàïèñü â êàíàë
					WaitForSingleObject(pipeWriter, INFINITE);							//Îæèäàíèå êîíöà çàïèñè â êàíàëà

					SetEvent(readyToRead);
					break;
				}

				WriteFile(hWrite, buf, read, NULL, &pipe);								//Çàïèñü â êàíàë
				WaitForSingleObject(pipeWriter, INFINITE);								//Îæèäàíèå êîíöà çàïèñè â êàíàëà

				SetEvent(readyToRead);

			} while (1);

			CloseHandle(hFile);

		} while (FindNextFile(hFind, &FindFileData));									//Ñëåäóþùèé ôàéë

		FindClose(hFind);
	}
	else {
	
		cout << "No txt files found in exe directory." << endl;
	
	}

	WaitForSingleObject(readyToWrite, INFINITE);										//Îæèäàíèå çàâåðøåíèÿ ÷òåíèÿ èç êàíàëà 

	SetEvent(finished);
	SetEvent(readyToRead);

	//Çàêðûòèå handl'îâ
	CloseHandle(hWrite);
	CloseHandle(finished);
	CloseHandle(readyToRead);
	CloseHandle(fileReader);
	CloseHandle(pipeWriter);
	CloseHandle(readyToWrite);

	return 0;
}

DWORD WINAPI writer(LPVOID lpParam) {

	HANDLE hRead = *(HANDLE*)lpParam;													//Äåñêèðèïòîð ÷òåíèÿ èç êàíàëà

	int read;																			//Êîë-âî ïðî÷èòàííûõ ñèìâîëîû
	const int bufSize = 100;															//Ðàçìåð áóôåðà
	char buf[bufSize];																	//Áóôåð äëÿ çàïèñè ñòðîêè																	

	//Ñîçäàíèå ñèíõðîíèçèðóþùåãî ñèãíàëà
	HANDLE readyToWrite;
	readyToWrite = CreateEvent(NULL, TRUE, FALSE, "readyToWrite");
	if (!readyToWrite) {
		cout << "Failed to create Event!" << endl;
		return 0;
	}

	//Overlapped äëÿ çàïèñè â ôàéë
	HANDLE fileWriter;
	fileWriter = CreateEvent(NULL, TRUE, TRUE, "fileWriter");
	if (!fileWriter) {
		cout << "Failed to create Event!" << endl;
		return 0;
	}

	OVERLAPPED file = { 0 };
	file.hEvent = fileWriter;

	//Overlapped äëÿ ÷òåíèÿ èç êàíàëà
	HANDLE pipeReader;
	pipeReader = CreateEvent(NULL, TRUE, TRUE, "pipeReader");
	if (!pipeReader) {
		cout << "Failed to create Event!" << endl;
		return 0;
	}

	OVERLAPPED pipe = { 0 };
	pipe.hEvent = pipeReader;

	//Îòêðûòèå ôàéëà äëÿ âûâîäà ðåçóëüòàòà
	HANDLE hFile;
	hFile = CreateFile("output.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	if (!hFile) {
		cout << "Failed to open a file!" << endl;
		return 0;
	}

	Sleep(100);

	//Îòêðûòèå ñèíõðîíèçèðóþùåãî ñèãíàëà
	HANDLE readyToRead = OpenEvent(EVENT_ALL_ACCESS, FALSE, "readyToRead");
	if (!readyToRead) {
		cout << "Failed to open readyToRead Event!" << endl;
		return 0;
	}

	//Îòêðûòèå ñèãíàëà îá îêîí÷àíèè ðàáîòû
	HANDLE finished = OpenEvent(SYNCHRONIZE, FALSE, "finished");
	if (!finished) {
		cout << "Failed to open finished Event!" << endl;
		return 0;
	}

	do {

		SetEvent(readyToWrite);															//Ðàçðåøåíèå çàïèñè â êàíàë

		WaitForSingleObject(readyToRead, INFINITE);										//Îæèäàíèå îêîí÷àíèÿ çàïèñè â êàíàë

			if (WaitForSingleObject(finished, 0) == WAIT_OBJECT_0) break;				//Ïðîâåðêà ñèãíàëà îá îêîí÷àíèè ðàáîòû

		ResetEvent(readyToRead);
		
		ReadFile(hRead, &buf, bufSize, NULL, &pipe);									//×òåíèå ñîîáùåíèÿ èç êàíàëà â áóôåð

		WaitForSingleObject(pipeReader, INFINITE);										//Îæèäàíèå êîíöà ÷òåíèÿ èç êàíàëà

		read = pipe.InternalHigh;														//Êîë-âî ïðî÷èòàííûõ ñèìâîëîâ

		WriteFile(hFile, buf, read, NULL, &file);										//Çàïèñü â ôàéë

		WaitForSingleObject(fileWriter, INFINITE);										//Îæèäàíèå êîíöà çàïèñè â ôàéë

		file.Offset += read;

	} while (1);

	//Çàêðûòèå handl'îâ
	CloseHandle(hFile);
	CloseHandle(hRead);
	CloseHandle(readyToWrite);
	CloseHandle(fileWriter);
	CloseHandle(pipeReader);
	CloseHandle(readyToRead);
	CloseHandle(finished);

	return 0;
}

//Âíåøíÿÿ ôóíêöèÿ
extern "C" __declspec(dllexport) void concat(void) {

	//Ñîçäàíèå àíîíèìíîãî êàíàëà
	HANDLE hWrite, hRead;
	if (!CreatePipe(&hRead, &hWrite, NULL, 0)) {
		cout << "Failed to create pipe!" << endl;
		return;
	}

	//Ñîçäàíèå ïîòîêà-ïèñàòåëÿ
	HANDLE hWriter;
	hWriter = CreateThread(NULL, 0, &writer, &hRead, 0, NULL);

	if (!hWriter) {
		cout << "Failed to create writer thread" << endl;
		return;
	}

	//Ñîçäàíèå ïîòîêà-÷èòàòåëÿ
	HANDLE hReader;
	hReader = CreateThread(NULL, 0, &reader, &hWrite, 0, NULL);

	if (!hReader) {
		cout << "Failed to create reader thread" << endl;
		TerminateThread(hWriter, 0);
		return;
	}


	WaitForSingleObject(hWriter, INFINITE);												//Îæèäàíèå îêî÷àíèÿ ðàáîòû ïîòîêà-ïèñàòåëÿ

	CloseHandle(hWrite);
	CloseHandle(hRead);
	CloseHandle(hReader);
	CloseHandle(hWriter);

	return;
}
