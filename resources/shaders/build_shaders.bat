@echo off

for %%v in (*.frag *.vert *.rchit *.rgen *.rmiss) do ( 
    "D:/VulkanSDK/1.2.162.0/Bin32/glslc.exe" "%%v" --target-env=vulkan1.2 -o "%%v".spv || goto :error
)

echo Shaders built successfully!
goto :end

:error
echo Error(s) building shaders.

:end