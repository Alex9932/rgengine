project "rg_modeleditor"
    rg_common()
    kind "SharedLib"

    files {
        "**.h", "**.cpp", "**.c",
        --path.join(RG_THIRDPARTY, "imgui", "backends", "imgui_impl_sdl3.cpp"),
        path.join(RG_THIRDPARTY, "imgui", "backends", "imgui_impl_opengl3.cpp")
    }

    rg_engine_includes()
    rg_vcpkg()

    links {
        "rgengine",
        "SDL3",
        "cjson"
    }

    filter "system:windows"
        filter "configurations:Debug"
            links { "assimp-vc145-mtd" }

        filter "configurations:Release"
            links { "assimp-vc145-mt" }

        filter {}

        links {
            "opengl32"
        }

    filter "system:linux"
        links {
            "assimp",
            "GL"
        }

    filter {}
