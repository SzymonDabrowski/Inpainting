#include "stdafx.h"
#include "Inpainting.h"
#include "C:\USERS\ASUS\vcpkg\installed\x64-windows\include\tiffio.h"
#include <Commdlg.h>
#include <string>
#include <stdlib.h>
#include <thread>
//#include <Windows.h>//multithread
//#include <process.h>//multithread
//#include <comdef.h>//to _bstr_t(wchar_t*)

#pragma warning(disable:4996)

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
TIFF *current_file;								// Current converted file
uint32 width;									// width of loaded image
uint32 height;									// height of loaded image
uint32 npixels;									// width * height
uint32 *raster;									//
const char* dcraw_path = "D:\\repos\\Inpainting\\dcraw"; 
												//put in folder with .exe and use relative path
//const char* command; // for multithreading, because void* to char* cast doesn't work 

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_INPAINTING, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_INPAINTING));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INPAINTING));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_INPAINTING);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//unsigned int __stdcall thread_convert(void* data)
//{
//	//const char* cmd = reinterpret_cast<const char*>(data);
//	//system(cmd);
//	system(command);
//	return 0;
//}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
			case IDM_OPEN:
			{
				if (current_file != NULL)
				{
					TIFFClose(current_file);
				}
				_TIFFfree(raster);

				#pragma region OpenFileDialog
				OPENFILENAME file;
				wchar_t file_name[260] = { 0 };
				LPCWSTR filter = L"RAW\0*.CR2\0";

				ZeroMemory(&file, sizeof(OPENFILENAME));
				file.lStructSize = sizeof(OPENFILENAME);
				file.hwndOwner = hWnd;
				file.lpstrFile = file_name; //file path here
				file.nMaxFile = 260;
				file.lpstrFilter = filter;
				file.nFilterIndex = 2;
				//file.lpstrFileTitle = NULL;
				//file.nMaxFileTitle = 0;
				file.lpstrInitialDir = NULL;
				file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

				GetOpenFileName(&file);
				wchar_t check[1] = { 0 };
				if (file.lpstrFile[0] == check[0])
				{
					break;
				}

				char path[260];
				std::wcstombs(path, file_name, 260);
				
				char* file_path = path;

				// Not needed because of prevoius if
				//if (file_path == NULL)
				//{
				//	break;
				//}
				#pragma endregion

				std::string cmd("d: && cd ");
				cmd.append(dcraw_path);
				cmd.append(" && dcraw -w -T -6 ");
				cmd.append(file_path); 

				//const char* command = cmd.c_str();
				const char* command = cmd.c_str();

				system(command); //can do something to hide cmd during converting?
				
				#pragma region multithread
				// Use this when using something else than system(const char*);
				//
				//HANDLE thread_cmd;
				//thread_cmd = (HANDLE)_beginthreadex(0, 0, &thread_convert, NULL, 0, 0);
				//WaitForSingleObject(thread_cmd, INFINITE);
				//
				//CloseHandle(thread_cmd);

				//change path to tiff file
				//
				//works only for cr2 and 3 digits extensions
				//use regex instead
				#pragma endregion

				std::string f_path(file_path);
				int size = f_path.length();
				for (int i = 0; i < size - 3; ++i)
				{
					if (f_path[i] == '.' && (f_path[i + 1] == 'C' || f_path[i + 1] == 'c') &&
						(f_path[i + 2] == 'R' || f_path[i + 2] == 'r') && f_path[i + 3] == '2')
					{
						f_path.replace(i + 1, i + 4, "TIFF");

						break;
					}
				}

				if ((current_file = TIFFOpen(f_path.c_str(), "r")) == NULL) {
					MessageBox(NULL, L"Cannot open .TIFF file", L"Loading error", MB_OK);
					break;
				}
		
				TIFFGetField(current_file, TIFFTAG_IMAGEWIDTH, &width);
				TIFFGetField(current_file, TIFFTAG_IMAGELENGTH, &height);

				npixels = width * height;
				raster = (uint32 *)_TIFFmalloc(npixels * sizeof(uint32));

				// Tiff file in RGBA raster
				TIFFReadRGBAImage(current_file, width, height, raster, 0);

				// NOW DISPLAY IMAGE! Pixel by pixel
				// ...
				// code here
				// ...
				// NOW PUT INVISIBLE MASK ON DISPLAYED IMAGE
				// ON THIS MASK USER WILL MARK WHERE TO INPAINT
				// ...
				// code here
				// ...

				break;
			}
			case IDM_SAVE:
			{
				MessageBox(NULL, L"Empty case IDM_SAVE", L"EMPTY", MB_OK);
				
				/*
				if (current_file == NULL)
				{
					break;
				}

				int sampleperpixel = 4; // 4 - RGBA, 3 - RGB
				char *image = new char[width*height*sampleperpixel];

				TIFFSetField(current_file, TIFFTAG_IMAGEWIDTH, width);					// set the width of the image
				TIFFSetField(current_file, TIFFTAG_IMAGELENGTH, height);				// set the height of the image
				TIFFSetField(current_file, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);	// set number of channels per pixel
				TIFFSetField(current_file, TIFFTAG_BITSPERSAMPLE, 8);					// (RAW was 14) set the size of the channels
				TIFFSetField(current_file, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);   // set the origin of the image.

				TIFFSetField(current_file, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);	// ? told it was essential
				TIFFSetField(current_file, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);		// ? same
				
				tsize_t linebytes = sampleperpixel * width;     // length in memory of one row of pixel in the image.
				unsigned char *buf = NULL;        // buffer used to store the row of pixel information for writing to file
												  // Allocating memory to store the pixels of current row
				
				if (TIFFScanlineSize(current_file) == linebytes)
				{
					buf = (unsigned char *)_TIFFmalloc(linebytes);
				}
				else
				{
					buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(current_file));
				}

				// set the strip size of the file to be size of one row of pixels
				TIFFSetField(current_file, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(current_file, width*sampleperpixel));

				// writing image to the file one strip at a time
				for (uint32 row = 0; row < height; row++)
				{
					memcpy(buf, &image[(height - row - 1)*linebytes], linebytes);
					if (TIFFWriteScanline(current_file, buf, row, 0) < 0)
						break;
				}
				
				// this is done while closing program or loading another image
				//TIFFClose(current_file);
				
				if (buf)
				{
					_TIFFfree(buf);
				}
				*/

				break;
			}
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_MAINWINDOW), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		_TIFFfree(raster);
		if (current_file != NULL)
		{
			TIFFClose(current_file);
		}
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
