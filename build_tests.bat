@echo off
setlocal enabledelayedexpansion

SET timestamp=%TIME:~3,2%%TIME:~6,2%
SET args=-g -DDEBUG
SET include_path=-I D:\Guigui\Work\Prog\_include\ -I %VULKAN_SDK%\include -I src/

SET linker_options=-L D:\Guigui\Work\Prog\_lib -L %VULKAN_SDK%\lib -Xlinker -incremental:no
SET libs=-lShell32.lib -lvulkan-1.lib

PUSHD tmp\
DEL /Q win32_*.pdb 2> NUL
POPD

ECHO Building test.exe
clang %args% %include_path% src/tests/unit_test.c -o tmp/test.exe %linker_options% %libs% -Xlinker -SUBSYSTEM:CONSOLE
IF !ERRORLEVEL! == 0 (
    ECHO BUILD OK
)
