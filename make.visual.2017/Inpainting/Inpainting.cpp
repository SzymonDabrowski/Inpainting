#include "Inpainting.h"
#include "CImg.h"
#include "mmreg.h"
#include <Commdlg.h>
#include <cstdio>
#include <string>
#include <stdlib.h>
#include <thread>
#include <windowsx.h>
#include <Windows.h>
#include <mmsystem.h>
#include "C:\USERS\ASUS\vcpkg\installed\x64-windows\include\tiffio.h"
#include "C:\USERS\ASUS\vcpkg\installed\x64-windows\include\opencv\cv.h"
#include "C:\Users\ASUS\vcpkg\installed\x64-windows\include\opencv2\imgproc.hpp"
#include "C:\Users\ASUS\vcpkg\installed\x64-windows\include\opencv2\highgui.hpp"

// TODO:
// cvReleaseImage(&image);
// lines: ~204, ~290, ~410

//disable warning about not safe functions
#pragma warning(disable:4996)

#pragma comment(lib,"winmm.lib")

#define MAX_LOADSTRING 100
#define IDB_INPAINT 101
#define M_PI 3.14159265358979323846

// Global Variables:
// WINDOW
HINSTANCE hInst;                    // current instance
WCHAR szTitle[MAX_LOADSTRING];      // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];// the main window class name
HWND buttonInpaint;

// IMAGE
TIFF *current_file;                 // Current converted file
uint32 width;                       // width of loaded image //uint32
uint32 height;                      // height of loaded image //uint32
uint32 npixels;                     // width * height
uint32 *raster;
cv::Size cv_size;			
IplImage *original_image;

// DECIMATION, DISPLAY, CONVERT
const char* dcraw_path = "D:\\repos\\Inpainting\\dcraw";// put in folder with .exe and use relative path
int decimate = 10;                  // 10 is ok, 8 not, make proportional to images size
unsigned char* rast;
unsigned char* mask;                // mask to draw on
IplImage *inpaint_mask;
int mouse_X_pos;
int mouse_Y_pos;
int brush_size = 5;                 // brush size used to inform where is the area to inpaint (ADD CONTROL)


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void				xDisplayBmpOnWindow(HWND hWnd, int iX, int iY, unsigned char* pRGBSample, int iWidth, int iHeight);
void				Decimate(unsigned int* original, unsigned char* decimated, int decimate);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
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

void xDisplayBmpOnWindow(HWND hWnd, int iX, int iY, unsigned char* pRGBSample, int iWidth, int iHeight)
{
	HDC hDC = GetDC(hWnd);
	HDC hDCofBmp = CreateCompatibleDC(hDC);

	HBITMAP hBmp = CreateCompatibleBitmap(hDC, iWidth, iHeight);

	SelectObject(hDCofBmp, hBmp);

	BITMAPINFOHEADER biBmpInfoHeader;

	biBmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
	biBmpInfoHeader.biWidth = iWidth;
	biBmpInfoHeader.biHeight = iHeight;
	biBmpInfoHeader.biPlanes = 1;
	biBmpInfoHeader.biBitCount = 32;//24
	biBmpInfoHeader.biCompression = BI_RGB;
	biBmpInfoHeader.biSizeImage = 0;
	biBmpInfoHeader.biXPelsPerMeter = 0;
	biBmpInfoHeader.biYPelsPerMeter = 0;
	biBmpInfoHeader.biClrUsed = 0;
	biBmpInfoHeader.biClrImportant = 0;

	SetDIBits(hDCofBmp, hBmp, 0, iWidth*iHeight, pRGBSample, (BITMAPINFO*)&biBmpInfoHeader, DIB_RGB_COLORS);

	BitBlt(hDC, iX, iY, iWidth, iHeight, hDCofBmp, 0, 0, SRCCOPY);

	SelectObject(hDCofBmp, 0);
	DeleteObject(hBmp);
	DeleteDC(hDCofBmp);

	ReleaseDC(hWnd, hDC);
}

void Decimate(unsigned int * original, unsigned char* decimated, int decimate)
{
	for (int y = 0; y < height / decimate; y++)
	{
		for (int x = 0; x < width / decimate; x++)
		{
			decimated[(y * width / decimate + x) * 4 + 2] = 
                ((unsigned char*)original)[(y * decimate * width + x * decimate) * 4 + 0]; //B
			decimated[(y * width / decimate + x) * 4 + 1] = 
                ((unsigned char*)original)[(y * decimate * width + x * decimate) * 4 + 1]; //G
			decimated[(y * width / decimate + x) * 4 + 0] = 
                ((unsigned char*)original)[(y * decimate * width + x * decimate) * 4 + 2]; //R
			decimated[(y * width / decimate + x) * 4 + 3] = 
                ((unsigned char*)original)[(y * decimate * width + x * decimate) * 4 + 3]; //A
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
	{                                                       //no WS_VISIBLE yet
		buttonInpaint = CreateWindow(L"button", L"Inpaint", WS_CHILD, 200, 370, 100, 30, hWnd, (HMENU) IDB_INPAINT, GetModuleHandle(NULL), NULL);
		break;
  }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
			case IDB_INPAINT:
			{
                //INPAINT HERE
                IplImage *inpainted;
                inpaint_mask->widthStep -= 2; //also hard coded, no time to code properly
                inpainted  = cvCreateImage(cvSize(width, height), 8, 3);
                cvInpaint(original_image, inpaint_mask, inpainted, 16, CV_INPAINT_TELEA); //3
                cvNamedWindow("Inpainted", CV_WINDOW_AUTOSIZE);
                cvShowImage("Inpainted", inpainted);
                //cvResizeWindow("Inpainted", 600, 400);
                //cvRelease()
                break; }
            case IDM_OPEN:
            {
	            if (mask != NULL) 
		            delete mask;

	            if (current_file != NULL) 
		            TIFFClose(current_file);

	            _TIFFfree(raster);

	            OPENFILENAME file;
	            wchar_t file_name[260] = { 0 };
	            LPCWSTR filter = L"RAW\0*.CR2\0";

	            ZeroMemory(&file, sizeof(OPENFILENAME));
	            file.lStructSize = sizeof(OPENFILENAME);
	            file.hwndOwner = hWnd;
	            file.lpstrFile = file_name; //file path
	            file.nMaxFile = 260;
	            file.lpstrFilter = filter;
	            file.nFilterIndex = 2;
	            //file.lpstrFileTitle = NULL;
	            //file.nMaxFileTitle = 0;
	            file.lpstrInitialDir = NULL;
	            file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	            GetOpenFileName(&file);
	            wchar_t check[1] = { NULL };
	            if (file.lpstrFile[0] == check[0])
	            {// break if no file was chosen
		            break;
	            }

	            char path[260];
	            std::wcstombs(path, file_name, 260);

	            char* file_path = path;

	            std::string cmd("d: && cd ");
	            cmd.append(dcraw_path);
	            cmd.append(" && dcraw -w -T ");// -6 when 16 bit depth needed
	            cmd.append(file_path); 

	            const char* command = cmd.c_str();
	            system(command); // hide cmd during converting?

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
	            cv_size.height = height;
	            cv_size.width = width;

	            npixels = width * height;
	            raster = (uint32 *)_TIFFmalloc(npixels * sizeof(uint32));
	            original_image = cvCreateImage(cvSize(width, height), 8, 3); 
	            inpaint_mask = cvCreateImage(cvSize(width, height), 8, 1);
	            cvSet(inpaint_mask, 0);
        
	            TIFFReadRGBAImage(current_file, width, height, raster, 0);

	            for (int y = 0; y < height; y++)
	            {
		            for (int x = 0; x < width; x++)
		            {	
			            original_image->imageData[(y * width + width - x) + 0] = 
				            ((unsigned char*)raster)[width*height * 4 - ((y * width + x) * 4 + 2)]; //R
			            original_image->imageData[(y * width + width - x) + 1] = 
				            ((unsigned char*)raster)[width*height * 4 - ((y * width + x) * 4 + 1)]; //B
			            original_image->imageData[(y * width + width - x) + 2] = 
				            ((unsigned char*)raster)[width*height * 4 - ((y * width + x) * 4 + 0)]; //G
		            }
	            }
	            original_image->widthStep -= 2; //may obtain the difference in widthSteps of this image and inpainted one
									            //in runtime. No time for that, hard coded when I saw this when debugging

	            // decimate with step that is dependent on file resolution
	            // rast = (unsigned char*)raster;
	            // 4770x3178 - and only this resolution atm
	            rast = new unsigned char[width / decimate * height / decimate * 4];
	            Decimate(raster, rast, decimate);
	            xDisplayBmpOnWindow(hWnd, 40, 40, rast, width / decimate, height / decimate); //display decimated

	            //make inpaint button active
	            ShowWindow(GetDlgItem(hWnd,IDB_INPAINT),SW_SHOW);
	            break;
            }
            case IDM_SAVE:
            {
	            //MessageBox(NULL, L"For now case IDM_SAVE is commented", L"EMPTY", MB_OK);
	            MessageBox(NULL, L"This is demo version. you can't save your image. Please, buy full version!", L"WARNING!", MB_OK);
				
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

	            TIFFSetField(current_file, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);	// ?
	            TIFFSetField(current_file, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);		// ?
				
	            tsize_t linebytes = sampleperpixel * width;     // length in memory of one row of pixel in the image.
	            unsigned char *buf = NULL;						// buffer used to store the row of pixel information for writing to file
													            // Allocating memory to store the pixels of current row
				
	            if (TIFFScanlineSize(current_file) == linebytes)
		            buf = (unsigned char *)_TIFFmalloc(linebytes);
	            else
		            buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(current_file));

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
		            _TIFFfree(buf);
				
	            break;
            }
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_MAINWINDOW), hWnd, About);
                break;
            case IDM_EXIT:
            {
	            DestroyWindow(hWnd);
	            WAVEFORMATEX pcmWaveFormat;
	            pcmWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	            pcmWaveFormat.nChannels = 1;
	            pcmWaveFormat.nSamplesPerSec = 44100L;
	            pcmWaveFormat.wBitsPerSample = 8;
	            pcmWaveFormat.nAvgBytesPerSec = 44100L;
	            pcmWaveFormat.nBlockAlign = 1;
	            pcmWaveFormat.cbSize = 0;

	            MMRESULT mmResult;
	            HWAVEOUT hwo = 0;
	            UINT devID;

	            for (devID = 0; devID < waveOutGetNumDevs(); devID++)
	            {
		            mmResult = waveOutOpen(&hwo, devID, &pcmWaveFormat, 0, 0, CALLBACK_NULL);
		            if (mmResult == MMSYSERR_NOERROR)
			            break;
	            }
	            if (mmResult != MMSYSERR_NOERROR)
	            {
		            MessageBox(hWnd, L"ERROR", TEXT("Nie znalaziono karty dzwiękwej"), MB_OK);
		            return mmResult;
	            }

	            WAVEHDR whdr;
	            ZeroMemory(&whdr, sizeof(WAVEHDR));
	            whdr.lpData = new char[pcmWaveFormat.nAvgBytesPerSec * 1];
	            whdr.dwBufferLength = pcmWaveFormat.nAvgBytesPerSec * 1;
	            whdr.dwUser = 0;
	            whdr.dwFlags = 0;
	            whdr.dwLoops = 0;
	            whdr.dwBytesRecorded = 0;
	            whdr.lpNext = 0;
	            whdr.reserved = 0;

	            for (int i = 0; i < whdr.dwBufferLength; ++i)
	            {
		            whdr.lpData[i] = (128 * sin((i * 440 * 2 * M_PI) / (double)pcmWaveFormat.nSamplesPerSec) + 128);
	            }

	            waveOutSetVolume(hwo, 0xFFFFFFFF);
	            mmResult = waveOutPrepareHeader(hwo, &whdr, sizeof(WAVEHDR));
	            if (mmResult != MMSYSERR_NOERROR)
	            {
		            MessageBox(hWnd, TEXT("Nie można zainicjować karty"), TEXT("ERROR"), MB_OK);
		            return mmResult;
	            }

	            mmResult = waveOutWrite(hwo, &whdr, sizeof(WAVEHDR));
	            if (mmResult != MMSYSERR_NOERROR)
	            {
		            MessageBox(hWnd, TEXT("Nie udało się załadować próbek"), TEXT("ERROR"), MB_OK);
		            return mmResult;
	            }

	            while ((whdr.dwFlags & WHDR_DONE) != WHDR_DONE)
	            {
		            Sleep(50);
	            }

	            mmResult = waveOutUnprepareHeader(hwo, &whdr, sizeof(WAVEHDR));
	            mmResult = waveOutClose(hwo);
	            delete[] whdr.lpData;

	            break;
            }
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        case WM_LBUTTONDOWN:
        {
	        // check if there is any image loaded
	        if (width == 0)
		        break;

	        //draw, where to inpaint
	        if (GET_X_LPARAM(lParam) > 40 && GET_X_LPARAM(lParam) < 40 + width / decimate
		        && GET_Y_LPARAM(lParam) > 40 && GET_Y_LPARAM(lParam) < 40 + height / decimate)
	        {
		        mouse_X_pos = GET_X_LPARAM(lParam) - 40;
		        mouse_Y_pos = height / decimate - GET_Y_LPARAM(lParam) + 40;

		        //display changes on screen
		        for (int y = 0; y < height / decimate; y++)
		        {
			        for (int x = 0; x < width / decimate; x++)
			        {
				        if (y <= mouse_Y_pos + brush_size && y >= mouse_Y_pos - brush_size &&
					        x <= mouse_X_pos + brush_size && x >= mouse_X_pos - brush_size)
				        {
					        rast[(y * width / decimate + x) * 4 + 2] = 0; //B
					        rast[(y * width / decimate + x) * 4 + 1] = 0; //G
					        rast[(y * width / decimate + x) * 4 + 0] = 0; //R
					        rast[(y * width / decimate + x) * 4 + 3] = 0; //A
				        }
			        }
		        }

		        //apply changes to mask
		        for (int y = 0; y < height; y++)
		        {
			        for (int x = 0; x < width; x++)
			        {
				        if (y <= (mouse_Y_pos + brush_size) * decimate && y >= (mouse_Y_pos - brush_size) * decimate &&
					        x <= (mouse_X_pos + brush_size) * decimate && x >= (mouse_X_pos - brush_size) * decimate)
					        inpaint_mask->imageData[(width * height - (y*width + width - x))/3] = 1; 
			        }
		        }

	        }
        }
        // to see changes immediately, keep this commented
        //break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            //display decimated picture here
            xDisplayBmpOnWindow(hWnd, 40, 40, rast, width / decimate, height / decimate);
            //xDisplayBmpOnWindow(hWnd, 40, 40, mask, width / decimate, height / decimate);

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