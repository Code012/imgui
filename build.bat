@echo off
setlocal

if not exist build mkdir build
pushd build

set code_path=../src/entry/

set output=main.exe
:: GENERAL COMPILER FLAGS
set compiler=               -nologo &:: Suppress Startup Banner
set compiler=%compiler%     -Oi     &:: Use assembly intrinsics where possible
set compiler=%compiler%     -MD     &:: Include CRT library in the executable (Dynamic link to match raylib.lib)
set compiler=%compiler%     -Gm-    &:: Disable minimal rebuild
set compiler=%compiler%     -GR-    &:: Disable runtime type info (C++)
set compiler=%compiler%     -EHa-   &:: Disable exception handling (C++)
set compiler=%compiler% 	-W4	    &:: So windows warnings go away
set compiler=%compiler% 	-WX	    &:: Treat all warnings as errors
set compiler=%compiler%		-I../src
:: IGNORE WARNINGS
set compiler=%compiler%     -wd4201 &:: Nameless struct/union
set compiler=%compiler%     -wd4100 &:: Unused function parameter
set compiler=%compiler%     -wd4189 &:: Local variable not referenced
set compiler=%compiler%     -wd4701 &:: Potentially uninitialized local variable 'name' used
set compiler=%compiler%     -wd4244 &:: conversion from 'U32' to 'U8', possible loss of data
set compiler=%compiler%     -wd4057 &:: 'initializing': 'const char *' differs in indirection to slightly different base types from 'U8 *'
set compiler=%compiler% 	-wd4090 &:: 'function': different 'const' qualifiers
set compiler=%compiler%		
:: DEBUG VARIABLES
set debug=		  			-FC &:: Produce the full path of the source code file
set debug=%debug% 			-Zi &:: Produce debug information (seperate PDB file, Use Z7 for embedding debug info into .obj file)
:: COMMON LINKER SWITCHES
set link=					-opt:ref				&:: Remove unused functions
set link=%link%		 		-incremental:no			&:: Perform full link each time
:: WIN32 PLATFORM LIBRARIES
set win32_libs=				user32.lib
set win32_libs=%win32_libs% Gdi32.lib
set win32_libs=%win32_libs% Winmm.lib
set win32_libs=%win32_libs% kernel32.lib
set win32_libs=%win32_libs% opengl32.lib
set win32_libs=%win32_libs% shell32.lib
set win32_libs=%win32_libs% Comdlg32.lib
:: CROSS_PLATFORM DEFINES
set defines=	      		-DBIND_INTERNAL=1
set defines=%defines% 		-DBIND_SLOW=1

cl -Od %compiler% %defines% %debug% -Fmbind.map %code_path%main.c /Fe:%output%  %win32_libs% /link %link% 

popd build
