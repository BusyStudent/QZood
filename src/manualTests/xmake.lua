add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.asan", "mode.profile")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
    add_cxxflags("cl::/Zc:__cplusplus")
    add_cxxflags("cl::/permissive-")
end 

if is_mode("release") then 
    set_symbols("hidden")
    set_strip("all")
    set_optimize("smallest")
end

add_requires("libxml2")
add_packages("libxml2")

includes("../ui")
includes("../common")
includes("../nekoav")

target("zoodWidgetTest")
    add_rules("qt.widgetapp")
    -- add_rules("protobuf.cpp")

    add_frameworks("QtCore", "QtGui", "QtWidgets")
    add_frameworks("QtOpenGL", "QtOpenGLWidgets")
    add_frameworks("QtNetwork")

    -- WebEngine
    -- add_frameworks("QtWebEngineCore", "QtWebChannel");

	add_deps("ui", "nekoav", "common")
	add_files("../../resources/resources.qrc")

    -- Main
    add_files("./*.cpp")
    add_files("./*.hpp")
    add_files("./*.ui")

    -- Player
    add_files("../player/*")

    -- Network
    add_files("../net/*")
    -- add_files("./src/net/protos/*")