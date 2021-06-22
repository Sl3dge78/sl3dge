@echo off
setlocal enabledelayedexpansion

set args= -GR- -EHa -nologo -Zi -experimental:external -external:anglebrackets -DDEBUG -Fotmp/ -Fdtmp/
set include_path=-external:I D:\Guigui\Work\Prog\_include\ -external:I %VULKAN_SDK%\include

set linker_options=-link -SUBSYSTEM:WINDOWS -LIBPATH:D:\Guigui\Work\Prog\_lib -LIBPATH:%VULKAN_SDK%\lib -incremental:no
set libs=SDL2main.lib SDL2.lib SDL2_image.lib Shell32.lib vulkan-1.lib

pushd tmp
del *.pdb > NUL 2> NUL
popd

cl %args% -Fewin32 %include_path% %vulkan_include_path% src/win32.cpp %linker_options% -PDB:tmp/ -OUT:bin/win32.exe %libs%

for %%G in (game, vulkan) do (
    set timestamp=%TIME:~3,2%%TIME:~6,2%
    cl %args% -Fe%%G %include_path% %vulkan_include_path% -LD src/%%G.cpp %linker_options% -PDB:tmp/%%G_%timestamp%.pdb -IMPLIB:tmp/%%G.lib -OUT:bin/%%G.dll %libs%
    if !ERRORLEVEL! == 0 ( echo a > bin/%%G.meta )

)

echo Done.