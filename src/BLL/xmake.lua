add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
end

includes("../common")

target("BLL")
    add_rules("qt.static")
    add_frameworks("QtCore", "QtGui", "QtWidgets")
    add_frameworks("QtOpenGL", "QtOpenGLWidgets", "QtNetwork")
    add_deps("nekoav", "common")
    add_files("data/*.cpp")

    add_files("manager/*.cpp")
    add_files("manager/*.hpp")
target_end()