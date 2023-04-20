project "RayTracer"
    kind "ConsoleApp"
    language "C++"

    targetdir("../bin/" .. outputdir .. "/%{prj.name}")
    objdir("../bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "source/**.cpp",
        "source/**.h"
    }

    defines { "WINDOWS" }

    filter { "configurations:Debug" }
        runtime "Debug"
        symbols "on"

    filter { "configurations:Release" }
        runtime "Release"
        optimize "on"