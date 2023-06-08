add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
    add_cxxflags("cl::/Zc:__cplusplus")
    add_cxxflags("cl::/permissive-")
end 

add_requires("libxml2", "protobuf-cpp")
add_packages("libxml2", "protobuf-cpp")

includes("./src/ui")
includes("./src/nekoav")

target("zood")
    add_rules("qt.widgetapp")
    add_rules("protobuf.cpp")

    add_frameworks("QtCore", "QtGui", "QtWidgets")
    add_frameworks("QtOpenGL", "QtOpenGLWidgets")
    add_frameworks("QtNetwork")

    -- WebEngine
    add_frameworks("QtWebEngineCore", "QtWebChannel");

	add_deps("ui", "nekoav")
	add_files("./resources/resources.qrc")

    -- Main
    add_files("./src/*.cpp")

    -- Player
    add_files("./src/player/*")

    -- Network
    add_files("./src/net/*")
    add_files("./src/net/protos/*")

    -- Tests
    add_files("./src/tests/*")
	add_files("./src/ui/common/tests/*")