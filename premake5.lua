workspace "rgengine"
    location "build"
    architecture "x86_64"
    configurations { "Debug", "Release" }
    startproject "entrypoint"

    filter "system:windows"
        systemversion "latest"

    filter {}

include "premake/common.lua"

include "rgengine"
include "rgrendervk"
--include "mmdlib"

include "rgtools"
include "rg_3da"
include "rg_leveleditor"
include "rg_modeleditor"
--include "rg_server"

include "entrypoint"

-- Windows only
filter "system:windows"
    include "rgrenderdx11"