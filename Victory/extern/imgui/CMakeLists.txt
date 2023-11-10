cmake_minimum_required(VERSION 3.0.0)
project(imgui)

set(imgui_files
	imconfig.h
	imgui.h
	imgui.cpp
	imgui_draw.cpp
	imgui_internal.h
	imgui_tables.cpp
	imgui_widgets.cpp
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
	imgui_demo.cpp
)

add_library(imgui ${imgui_files})
set_target_properties(imgui
    PROPERTIES
        CXX_STANDARD 17
)