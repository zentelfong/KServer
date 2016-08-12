
workspace "KServer"
	configurations { "Debug", "Release" }
	platforms { "Win32", "Win64", "Linux" }
	location "build"

filter { "platforms:Win32" }
    system "windows"
    architecture "x32"
	defines { "_WIN32", "WIN32" }
	
filter { "platforms:Win64" }
    system "windows"
    architecture "x64"	
	defines { "_WIN32", "WIN32" }
	
filter { "platforms:Linux" }
    system "linux"
    architecture "x64"	
	defines { "LINUX", "linux" ,"POSIX"}
	
	
	
filter "configurations:Debug"
	defines { "DEBUG" }
	flags { "Symbols" }
	optimize "Debug"
	
filter "configurations:Release"
	defines { "NDEBUG" }
	optimize "On"
	flags { "OptimizeSpeed", "EnableSSE2" }
	

project "KServer"
	kind "ConsoleApp"
	language "C++"
	
	includedirs{"./"}
	
	files { 
		"*.h", "*.cpp",
		"*.c",
	}


