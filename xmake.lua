add_rules("mode.debug", "mode.release", "mode.releasedbg")

set_languages("c++17")

if is_host("windows") then
    add_cxxflags("cl::/utf-8")
end 

add_requires("qt5base", "libxml2")
add_packages("qt5base", "libxml2")


target("zood")
    add_rules("qt.widgetapp")

    add_frameworks("QtCore", "QtGui", "QtWidgets")
    add_frameworks("QtMultimedia", "QtMultimediaWidgets")
    add_frameworks("QtNetwork")

    -- Main
    add_files("./src/*.cpp")

    -- Player
    add_files("./src/player/*")

    -- Tests
    add_files("./src/tests/*")