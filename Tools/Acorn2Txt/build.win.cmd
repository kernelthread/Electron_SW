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
cl /nologo /O2 acorn2txt.cpp
@goto :eof

:gcc
@echo Building with GCC
g++ -O2 -o acorn2txt.exe acorn2txt.cpp
@goto :eof

:search
@set __searchres=%~$PATH:1

:eof
