@echo off
setlocal

set SHADER_DIR=assets\engine\shaders\vulkan
set GLSLC="C:\VulkanSDK\1.4.350.0\Bin\glslc.exe"

echo Compiling Vulkan shaders...

if not exist "%SHADER_DIR%" (
    echo Directory %SHADER_DIR% does not exist! Creating it...
    mkdir "%SHADER_DIR%"
)

for %%f in ("%SHADER_DIR%\*.vert" "%SHADER_DIR%\*.frag") do (
    echo Compiling %%f to %%f.spv...
    %GLSLC% "%%f" -o "%%f.spv"
    if errorlevel 1 (
        echo Failed to compile %%f
        exit /b 1
    )
)

echo All shaders compiled successfully!
endlocal
exit /b 0
