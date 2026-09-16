project "rgrenderdx11"
    rg_common()
    kind "SharedLib"

    files {
        "**.h",
        "**.cpp",
        path.join(RG_THIRDPARTY, "imgui", "backends", "imgui_impl_dx11.cpp")
    }

    rg_engine_includes()
    rg_vcpkg()

    links {
        "rgengine",
        "SDL3",
        "d3d11",
        "dxgi",
        "dxguid"
    }
