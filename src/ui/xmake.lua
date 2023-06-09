add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
end 

includes("../BLL")

target("ui")
 	add_rules("qt.static")
	add_frameworks("QtCore", "QtGui", "QtWidgets")
    add_frameworks("QtOpenGL", "QtOpenGLWidgets")

    add_deps("BLL");

	add_headerfiles("./common/*.hpp")
	add_files("./common/*.cpp")
	add_files("./common/*.hpp")
	add_files("./zood/*.cpp")
	add_files("./zood/*.hpp")
    add_files("./player/*.hpp")
    add_files("./player/*.cpp")
    add_files("./player/*.ui")
    add_files("./zood/*.ui")
target_end()