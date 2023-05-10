add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
end 

add_requires("qt5base", "libxml2")
add_packages("qt5base", "libxml2")

target("ui")
 	add_rules("qt.static")
	add_frameworks("QtCore", "QtGui", "QtWidgets")

	add_headerfiles("./common/*.hpp")
	add_files("./common/*.cpp")
	add_files("./common/*.hpp")
	add_files("./zood/*.cpp")
	add_files("./zood/*.hpp")
target_end()