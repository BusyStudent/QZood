add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
end 

includes("../common")

target("net")
 	add_rules("qt.static")

	add_frameworks("QtCore", "QtGui", "QtWidgets")
    add_frameworks("QtNetwork")

    add_deps("common")

	add_files("./*.cpp")
	add_files("./*.hpp")