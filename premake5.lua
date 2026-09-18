include "premake/common.lua"

workspace "rgengine"
    location "build"
    architecture "x86_64"
    configurations { "Debug", "Release" }
    startproject "entrypoint"
    debugdir(path.join(RG_ROOT, "resources"))

    filter "system:windows"
        systemversion "latest"

    filter {}

include "rgengine"
include "rgrendervk"

include "rgtools"
include "rg_3da"
include "rg_leveleditor"
include "rg_modeleditor"

include "entrypoint"

-- Windows only
filter "system:windows"
    include "rgrenderdx11"