if is_plat("linux") then 
    -- Use ffmpeg from system
    add_requires("libavformat", "libavutil", "libavcodec", "libswresample", "libswscale")
    -- Use SDL by default
    add_requires("libsdl")
else 
    -- Use ffmpeg from repo
    add_requires("ffmpeg");
    -- Use miniaudio by default
    add_requires("miniaudio")
end

target("nekoav")
    add_rules("qt.static")

    add_frameworks("QtCore", "QtWidgets", "QtGui")

    if is_plat("linux") then 
        -- Use ffmpeg from system
        add_packages("libavformat", "libavutil", "libavcodec", "libswresample", "libswscale")
        add_packages("libsdl")
    else 
        add_packages("ffmpeg")
        add_packages("miniaudio")

        -- Tell it
        add_defines("NEKOAV_MINIAUDIO")
    end

    add_files("*.cpp")
    add_files("*.hpp")

    set_license("GPL-2.0")
target_end()
