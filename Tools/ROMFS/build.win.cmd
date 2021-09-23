@setlocal
@call :search cl.exe
@if "%__searchres%"=="" (
    goto :gcc
) else (
    goto :msvc
)
@endlocal
@goto :eof

:msvc
@echo Building with MSVC
cl /nologo /O2 build_romfs.cpp
@goto :eof

:gcc
@echo Building with GCC
g++ -o build_romfs.exe build_romfs.cpp
@goto :eof

:search
@set __searchres=%~$PATH:1

:eof
