project "rg_3da"
    rg_common()
    kind "SharedLib"

    files { "**.h", "**.cpp" }

    rg_engine_includes()
    rg_vcpkg()

    links {
        "rgengine",
        "SDL3"
    }
