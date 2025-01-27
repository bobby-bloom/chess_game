project "Client-Windows"
   kind "WindowedApp"
   language "C"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { 
    "Source/**.h", 
    "Source/**.c", 
    "Source/Headers/**.h", 
    "Source/Resources/**.rc" ,
   }

   includedirs {
      "Source",
      "Source/Headers",
      "Source/Resources",

	  -- Include Core
	  "../Chess-Core/Source",
	  "../Chess-Core/Source/Headers",

	  -- Vendor / Third Party
      "vcpkg_installed/x64-windows/x64-windows/include",
      "vcpkg_installed/x64-windows/x64-windows/include/cairo",
      "vcpkg_installed/x64-windows/x64-windows/include/gdk-pixbuf-2.0",
      "vcpkg_installed/x64-windows/x64-windows/include/gio-win32-2.0",
      "vcpkg_installed/x64-windows/x64-windows/include/librsvg-2.0/librsvg",
      "vcpkg_installed/x64-windows/x64-windows/include/glib-2.0",
      "vcpkg_installed/x64-windows/x64-windows/lib/glib-2.0/include",
   }
   
   libdirs {"vcpkg_installed/x64-windows/x64-windows/lib"}

   links {
      "Chess-Core",
      "cairo", 
      "rsvg-2",
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"