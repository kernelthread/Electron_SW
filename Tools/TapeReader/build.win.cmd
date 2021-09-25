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
cl /nologo /O2 /Fe:tape_reader.exe tape_reader.cpp wav.cpp demod.cpp decoder.cpp
@goto :eof

:gcc
@echo Building with GCC
g++ -Ofast -o tape_reader.exe tape_reader.cpp wav.cpp demod.cpp decoder.cpp -lm
@goto :eof

:search
@set __searchres=%~$PATH:1

:eof
