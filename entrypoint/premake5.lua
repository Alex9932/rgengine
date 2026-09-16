project "entrypoint"
    rg_common()
    --kind "WindowedApp"
    kind "ConsoleApp"

    debugdir(path.join(RG_ROOT, "resources"))

    files { "**.h", "**.cpp" }

    rg_engine_includes()
    rg_vcpkg()
    rg_vcpkg_runtime()

    links { "rgengine" }