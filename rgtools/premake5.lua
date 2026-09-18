project "rgtools"
    rg_common()
    kind "ConsoleApp"

    files { "**.h", "**.cpp" }

    rg_engine_includes()
    rg_vcpkg()

    includedirs {
        path.join(RG_THIRDPARTY, "spirv-cross")
    }

    files {
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_cfg.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_cpp.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_cross.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_cross_c.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_cross_parsed_ir.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_cross_util.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_glsl.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_hlsl.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_msl.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_parser.cpp"),
        path.join(RG_THIRDPARTY, "spirv-cross", "spirv_reflect.cpp")
    }

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