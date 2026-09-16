project "rg_leveleditor"
    rg_common()
    kind "SharedLib"

    files { "**.h", "**.cpp" }

    rg_engine_includes()
    rg_vcpkg()

    links {
        "rgengine",
        "SDL3"
    }
