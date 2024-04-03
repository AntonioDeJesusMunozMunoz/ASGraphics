#include <fileLoader.hpp>
#include <vector>
unsigned char readBinFile(std::string path) {
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
	//pedir de python la longitud del archivo
	int longitudDelArchivo;
	printf("\nc waiting for filesize");
	WaitForSingleObject(pi.hProcess, 2000);
	printf("\nc reading filesize: ");
	ReadFile(read, &longitudDelArchivo, sizeof(longitudDelArchivo), &bytesRead, NULL);
	std::cout << bytesRead << std::endl;

	WriteFile(write, "\n", 1, &bytesWritten, NULL);

	//crear array de esa longitud
	std::vector <unsigned char> datos(longitudDelArchivo);
	
	//conseguir de python los datos
	ReadFile(read, datos.data(), sizeof(unsigned char) * longitudDelArchivo, &bytesRead, NULL);
	std::cout << bytesRead << std::endl;

	printf("waiting\n");
	WaitForSingleObject(pi.hProcess, INFINITE);

	printf("done waiting");
	CloseHandle(read);
	CloseHandle(write);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

		
	return 's';

}
