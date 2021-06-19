@echo off

pushd resources\shaders\
for %%v in (*.frag *.vert *.rchit *.rgen *.rmiss) do (
    echo %%v
    "D:/VulkanSDK/1.2.162.0/Bin32/glslc.exe" "%%v" --target-env=vulkan1.2 -o "%%v".spv || goto :error
)

echo a > shaders.meta
echo Shaders built successfully!
popd
exit -b 0

:error
echo Error(s) building shaders.
popd
exit -b 1