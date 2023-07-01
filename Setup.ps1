# See if vcpkg is available
if ($null -eq (Get-Command "vcpkg" -ErrorAction SilentlyContinue))
{
    Write-Host "vcpkg not found. Download it from https://github.com/microsoft/vcpkg"
    exit
}

& vcpkg install glad:x64-windows
& vcpkg install glfw3:x64-windows
& vcpkg install glm:x64-windows
& vcpkg install stb:x64-windows