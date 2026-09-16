project "rgengine"
    rg_common()
    kind "SharedLib"

    files {
        "**.h",
        "**.cpp",
    }

    rg_engine_includes()
    rg_engine_imgui_cpp()
    rg_vcpkg()

    filter "system:windows"
        links {
            "SDL3",
            "mujs",
            "cjson",
            "OpenAL32",
            "nfd",
            "iconv",
            "user32",
            "shell32",
            "ole32",
            "oleaut32",
            "imm32",
            "version"
        }

        filter "configurations:Release"
            links {
                "freetype",
                "z",

                "Bullet3Common",
                "BulletCollision",
                "BulletDynamics",
                "BulletInverseDynamics",
                "BulletSoftBody",
                "LinearMath",
            }

        filter "configurations:Debug"
            links {
                "freetyped",
                "zd",
                
                "Bullet3Common_Debug",
                "BulletCollision_Debug",
                "BulletDynamics_Debug",
                "BulletInverseDynamics_Debug",
                "BulletSoftBody_Debug",
                "LinearMath_Debug"
            }

        filter {}
    
    filter "system:linux"
        links {
            "SDL3",
            "mujs",
            "cjson",
            "freetype",
            "OpenAL32",
            "z",

            "Bullet3Common",
            "BulletCollision",
            "BulletDynamics",
            "BulletInverseDynamics",
            "BulletSoftBody",
            "LinearMath",

            "dl",
            "pthread"
        }
    
    filter {}