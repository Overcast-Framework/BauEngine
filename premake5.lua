workspace "BauEngine"
   configurations { "Debug", "Release" }

project "BauEngine"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"
   
   targetdir "bin/%{cfg.buildcfg}"

   files { "src/**.h", "src/**.cpp" }

   includedirs { "vendors/DirectXTK12/Inc", "vendors/glm", "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um" }

   libdirs { "vendors/DirectXTK12/Bin/Desktop_2022_Win10/x64/Release", os.findlib("d3d12") }
   links { "DirectXTK12", "d3d12", "dxgi", "d3dcompiler", "user32", "gdi32", "shell32" }

   filter "configurations:Debug"
      defines { "DEBUG", "D3D12_DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"