10.10.2018

Though there are some bugs to fix, program works properly: it inpaints data in places marked by user.
This was the main purpose of writing this application and it is done. Due to lack of time it will
stay as is for some time.

Changes:
- tabs changed to spaces
- added "/Y-" to Debug->Properties->Configuration Properties->C/C++->Command Line->Additional Options
- removed "stdafx/h" from project and includes	
- added gitignore

Fixed problems:
- cursive image (inpainted image preview) is now straight
- cursive mask (as above)
- too big mask applied to the image
- upside-down and left-right reversed image
- upside-down and left-right reversed mask

Finded bugs:
- can inpaint once in runtime
- image is still black and white only

Still to solve
- cursive images fix is hard-coded. Obtain widthStep at runtime and then apply changes
- decimation is made for one resolution only (fixed for my Canon 500d camera)


***


08.07.2018

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




