project "rgrendervk"
    rg_common()
    kind "SharedLib"

    files {
        "**.h",
        "**.cpp",
        path.join(RG_THIRDPARTY, "imgui", "backends", "imgui_impl_vulkan.cpp")
    }

    rg_engine_includes()
    rg_vcpkg()

    includedirs {
        path.join(RG_THIRDPARTY, "VulkanMemoryAllocator", "include"),
        path.join(RG_THIRDPARTY, "imgui")
    }

    filter "system:windows"
        local sdk = os.getenv("VULKAN_SDK")

        if sdk then
            includedirs(path.join(sdk, "Include"))
            libdirs(path.join(sdk, "Lib"))
        end

        links {
            "vulkan-1",
            "SDL3",
            "rgengine"
        }

    filter "system:linux"
        links {
            "vulkan",
            "SDL3",
            "rgengine"
        }

    filter {}
