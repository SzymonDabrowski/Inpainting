Libraries installed using "vcpkg install library_name:x64-windows"		
										library_name:
#include "C:\USERS\ASUS\vcpkg\installed\x64-windows\include\tiffio.h"		[tiff or libtiff]
#include "C:\USERS\ASUS\vcpkg\installed\x64-windows\include\opencv\cv.h"	[opencv]

====================================================================================================

CImg and Inpainting.h may be not needed, but I can't check this, due to end of my trial period on VS. 
At this moment this is the actual version. Soon I'll take care of problems with displaying inpainted
image and finish saving module.

Note about line 48: const char* dcraw_path = "D:\\repos\\Inpainting\\dcraw";
I worked with my laptop only and did not need to use relative path at all, this worked perfectly. If 
you use it on your computer, change the path to your absolute or relative.

====================================================================================================

In case of problems with .dll files, let me know. I'll try to help you with this.

Szymon D¹browski, 08.07.2018
