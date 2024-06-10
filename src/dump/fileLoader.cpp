#include <fileLoader.hpp>
#include <vector>
void readBinFile(std::string path, std::vector <unsigned char> *returnData) {
	//crear subproceso de python
	HANDLE read, write;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	STARTUPINFO si;

	//crear pipes
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&read, &write, &sa, 0)) {
		printf("\nno pipe created");
	}

	//inicializar attributos del subproceso
	HANDLE cppStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.hStdInput = read;
	si.hStdOutput = write;
	si.hStdError = cppStdOut;
	si.dwFlags |= STARTF_USESTDHANDLES;

	if (!CreateProcess("./venv/Scripts/python.exe", (LPSTR)"  ./pysrc/fileLoader.py", NULL, &sa, TRUE, 0, NULL, NULL, &si, &pi)) {
		printf("NO\n");
	}
	else {
		printf("si\n");
	}

	printf("error: %u\n", GetLastError());


	DWORD bytesRead, bytesWritten;

	//mandando a python el archivo a leer
	WriteFile(write, path.c_str(), strlen(path.c_str()), &bytesWritten, NULL);

	//pedir de python la longitud del archivo
	int longitudDelArchivo;
	printf("\nc waiting for filesize");
	WaitForSingleObject(pi.hProcess, 2000);
	printf("\nc reading filesize: ");
	ReadFile(read, &longitudDelArchivo, sizeof(longitudDelArchivo), &bytesRead, NULL);
	printf("bytes read: %u, meaning %i\n");
	printf("\nc greenligthing ");
	WriteFile(write, "\n", 1, &bytesWritten, NULL);

	//darle al array esa longitud
	//returnData->assign(longitudDelArchivo, (unsigned char)'a');
	returnData->resize(longitudDelArchivo);
	//conseguir de python los datos
	ReadFile(read, returnData->data(), sizeof(unsigned char) * longitudDelArchivo, &bytesRead, NULL);
	printf("bytes read: %u\n");

	printf("waiting\n");
	WaitForSingleObject(pi.hProcess, INFINITE);

	printf("done waiting");
	CloseHandle(read);
	CloseHandle(write);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

		
}
