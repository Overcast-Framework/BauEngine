workspace "BauEngine"
   configurations { "Debug", "Release" }
   platforms { "Win64" }

filter { "platforms:Win64" }
    system "Windows"
    architecture "x86_64"

project "BauEngine"
   kind "StaticLib"
   language "C++"
   cppdialect "C++17"
   
   targetdir "bin/%{cfg.buildcfg}"

   files { "src/**.h", "src/**.cpp" }

   pchheader "bepch.h"
   pchsource "src/bepch.cpp"

   includedirs { "vendors/DirectXTK12/Inc", "src", "vendors/glm", "vendors/stb", "%WindowsSdkDir/Include%WindowsSDKVersion%/um", "vendors/DirectX-Headers/include", "vendors/glfw/include" }

   libdirs { "vendors/DirectXTK12/Bin/Desktop_2022_Win10/x64/Release", "%WindowsSdkDir/Lib%WindowsSDKVersion%/um/arch", "vendors/DirectX-Headers/Debug", "vendors/glfw/src/Release" }
   links { "DirectXTK12", "d3d12", "dxguid", "dxgi", "d3dcompiler", "user32", "opengl32.lib", "gdi32", "shell32", "DirectX-Headers", "DirectX-Guids", "glfw3" }

   filter "configurations:Debug"
      defines { "DEBUG", "GLFW_STATIC", "D3D12_DEBUG", "GLFW_EXPOSE_NATIVE_WIN32" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG", "GLFW_STATIC", "GLFW_EXPOSE_NATIVE_WIN32" }
      optimize "On"

project "Bauer"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"
   
   targetdir "Bauer/bin/%{cfg.buildcfg}"

   files { "Bauer/src/**.h", "Bauer/src/**.cpp" }
   includedirs { "vendors/DirectXTK12/Inc", "src", "vendors/glm", "%WindowsSdkDir/Include%WindowsSDKVersion%/um", "vendors/DirectX-Headers/include", "vendors/glfw/include" }

   libdirs { "vendors/DirectXTK12/Bin/Desktop_2022_Win10/x64/Release", "%WindowsSdkDir/Lib%WindowsSDKVersion%/um/arch", "vendors/DirectX-Headers/Debug", "vendors/glfw/src/Release", "bin/%{cfg.buildcfg}" }
   links { "BauEngine", "glfw3", "d3d12", "dxguid", "dxgi", "opengl32.lib", "d3dcompiler", "DirectX-Headers", "DirectX-Guids" }

   filter "configurations:Debug"
      defines { "DEBUG", "GLFW_STATIC" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG", "GLFW_STATIC" }
      optimize "On"