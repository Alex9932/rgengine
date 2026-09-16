project "rg_modeleditor"
    rg_common()
    kind "SharedLib"

    files {
        "**.h", "**.cpp", "**.c",
        path.join(RG_THIRDPARTY, "imgui", "backends", "imgui_impl_sdl3.cpp"),
        path.join(RG_THIRDPARTY, "imgui", "backends", "imgui_impl_opengl3.cpp")
    }

    rg_engine_includes()
    rg_vcpkg()

    links {
        "rgengine",
        "SDL3",
        "cjson",
        "assimp"
    }

    filter "system:windows"
        links {
            "opengl32"
        }

    filter "system:linux"
        links {
            "GL"
        }

    filter {}
