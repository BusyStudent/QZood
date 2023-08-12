add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
end

includes("../../common")

target("widgetUtil")
 	add_rules("qt.static")
	add_frameworks("QtCore", "QtGui", "QtWidgets")
    add_frameworks("QtOpenGL", "QtOpenGLWidgets")

    add_deps("common")

    add_files("./layout/*.cpp")
    add_files("./layout/*.hpp")
    add_files("./widget/*.cpp")
    add_files("./widget/*.hpp")
    add_files("./interface/*.hpp")
target_end()