@echo off
set project_name=win32
set timestamp=%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%

set args= -GR- -EHa -nologo -Zi -experimental:external -external:anglebrackets -DDEBUG -Fotmp/ -Fdtmp/
set include_path=-external:I D:\Guigui\Work\Prog\_include\ -external:I %VULKAN_SDK%\include

set linker_options=-link -SUBSYSTEM:WINDOWS -LIBPATH:D:\Guigui\Work\Prog\_lib -LIBPATH:%VULKAN_SDK%\lib -incremental:no
set libs=SDL2main.lib SDL2.lib SDL2_image.lib Shell32.lib vulkan-1.lib

pushd tmp
del *.pdb > NUL 2> NUL
popd

cl %args% -Fe%project_name% %include_path% %vulkan_include_path% src/win32.cpp %linker_options% -PDB:tmp/ -OUT:bin/win32.exe %libs%
cl %args% -Fegame %include_path% %vulkan_include_path% -LD src/game.cpp %linker_options% -PDB:tmp/game_%timestamp%.pdb -IMPLIB:tmp/game.lib -OUT:bin/game.dll %libs%
echo a > bin/game.meta
cl %args% -Fevulkan %include_path% %vulkan_include_path% -LD src/vulkan.cpp %linker_options% -PDB:tmp/game_%timestamp%.pdb -IMPLIB:tmp/vulkan.lib -OUT:bin/vulkan.dll %libs%
echo a > bin/vulkan.meta
echo Build completed!
