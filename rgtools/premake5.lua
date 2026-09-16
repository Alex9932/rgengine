project "rgtools"
    rg_common()
    kind "ConsoleApp"

    files { "**.h", "**.cpp" }

    rg_engine_includes()
    rg_vcpkg()

    filter "system:windows"
        links {
            "rgengine",
            "SDL3",
            "dxguid",
            "dxgi",
            "d3dcompiler"
        }

    filter "system:windows"
        links {
            "rgengine",
            "SDL3"
        }

    filter {}