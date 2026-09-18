RG_ROOT = path.getabsolute(_MAIN_SCRIPT_DIR)
RG_THIRDPARTY = path.join(RG_ROOT, "thirdparty")

function rg_common()
    language "C++"
    cppdialect "C++20"
    characterset "Unicode"

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        symbols "On"
        runtime "Debug"
        defines { "_DEBUG" }

    filter "configurations:Release"
        optimize "Full"
        symbols "On"
        runtime "Release"
        defines { "NDEBUG" }

    filter {}
end

function rg_vcpkg()
    filter "system:windows"
        local vcpkg = path.join(RG_ROOT, "vcpkg_installed", "x64-windows")

        includedirs {
            path.join(vcpkg, "include")
        }

        filter "configurations:Debug"
            libdirs {
                path.join(vcpkg, "debug", "lib")
            }

        filter "configurations:Release"
            libdirs {
                path.join(vcpkg, "lib")
            }
        
    filter {}
end

function rg_vcpkg_runtime()
    filter "system:windows"
        local vcpkg = path.join(RG_ROOT, "vcpkg_installed", "x64-windows")

        filter "configurations:Debug"
            postbuildcommands {
                "{COPYDIR} " ..
                    path.join(vcpkg, "debug", "bin") ..
                    " " ..
                    --path.join("%{cfg.targetdir}")
                    path.join(RG_ROOT, "build", "bin", "Debug")
            }

        filter "configurations:Release"
            postbuildcommands {
                "{COPYDIR} " ..
                    path.join(vcpkg, "bin") ..
                    " " ..
                    --path.join("%{cfg.targetdir}")
                    path.join(RG_ROOT, "build", "bin", "Release")

            }

        filter {}
end

function rg_engine_includes()
    defines {
        "IMGUI_API=__declspec(dllimport)"
    }
    includedirs {
        path.join(RG_ROOT, "rgengine", "include"),
        path.join(RG_ROOT, "rgengine"),
        path.join(RG_THIRDPARTY, "stb"),
        path.join(RG_THIRDPARTY, "imgui")
    }
end

function rg_engine_imgui_cpp()
    defines {
        "IMGUI_API=__declspec(dllexport)"
    }
    files {
        path.join(RG_THIRDPARTY, "imgui", "imgui.cpp"),
        path.join(RG_THIRDPARTY, "imgui", "imgui_draw.cpp"),
        path.join(RG_THIRDPARTY, "imgui", "imgui_tables.cpp"),
        path.join(RG_THIRDPARTY, "imgui", "imgui_widgets.cpp"),
        path.join(RG_THIRDPARTY, "imgui", "imgui_demo.cpp"),
        path.join(RG_THIRDPARTY, "imgui", "backends", "imgui_impl_sdl3.cpp")
    }
end
