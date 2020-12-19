@echo off

for %%v in (*.frag *.vert) do ( 
    "D:/VulkanSDK/1.2.154.1/Bin32/glslc.exe" "%%v" -o "%%v".spv || goto :error
)

echo Shaders built successfully!
goto :end

:error
echo Error(s) building shaders.

:end