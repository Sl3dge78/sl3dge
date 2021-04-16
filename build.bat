@echo off
set project_name=vulkan

set args= -GR- -EHa -nologo -Zi -experimental:external -external:anglebrackets -DDEBUG
set include_path=-external:I ..\include\ -external:I %VULKAN_SDK%\include

set linker_options=-link -SUBSYSTEM:WINDOWS -LIBPATH:..\lib -LIBPATH:%VULKAN_SDK%\lib
set libs=SDL2main.lib SDL2.lib SDL2_image.lib Shell32.lib vulkan-1.lib

pushd bin
cl %args% -Fe%project_name% %include_path% %vulkan_include_path% ../src/win32.cpp %linker_options% %libs%
cl %args% -Fegame %include_path% %vulkan_include_path% -LD ../src/game.cpp %linker_options% %libs%
popd
