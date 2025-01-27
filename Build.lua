workspace "Chess Game"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Client-Windows"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Core"
	include "Chess-Core/Build-Core.lua"
group ""

include "Client-Windows/Build-Client.lua"
include "Client-Linux/Build-Client.lua"
