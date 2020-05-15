#include <windows.h>
#include <iostream>

using namespace std;

int main() {

	HMODULE dll;

	void (*concat)(void);												//Óêàçàòåëü íà ôóíêöèþ

	dll = LoadLibrary("concatFiles.dll");								//Çàãðóçêà äèíàìè÷åñêîé áèáëèîòåêè
	if (!dll) {

		cout << "Failed to load DLL" << endl;
		return GetLastError();
	}

	concat = (void (*)(void))GetProcAddress(dll, "concat");				//Ïîëó÷åíèå àäðåñà ôóíêöèè
	if (!concat){

		cout << "Failed to get DLL function" << endl;
		return GetLastError();
	}

	concat();															//Âûçîâ ôóíêöèè êîíêàòåíàöèè ôàéëîâ	

	//Âûãðóçêà áèáëèîòåêè
	if (!FreeLibrary(dll)){		

		cout << "Failed to free DLL" << endl;
		return GetLastError();
	}

	return 0;
}
