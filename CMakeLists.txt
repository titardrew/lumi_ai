cmake_minimum_required (VERSION 3.14)

project(OpenVINO_YOLO)

find_package(OpenVINO HINTS Release REQUIRED)

add_executable(yolo_example source/main.cc)
target_link_libraries(yolo_example PUBLIC openvino::runtime)

add_library(Lumi STATIC source/lumi.cc)
target_link_libraries(Lumi PUBLIC openvino::runtime)

if (APPLE)
    # OSX Bundle
    add_library(OVYoloCpp MODULE source/ov_yolo.cc)
    target_link_libraries(OVYoloCpp PUBLIC openvino::runtime Lumi)
    set_target_properties(OVYoloCpp PROPERTIES
        BUNDLE True
        MACOSX_BUNDLE_GUI_IDENTIFIER my.domain.style.identifier.OVYoloCpp
        MACOSX_BUNDLE_BUNDLE_NAME OVYoloCpp
        MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in
    )
elseif(WIN32)
    # DLL
    add_library(OVYoloCpp SHARED source/win/dllmain.cpp source/win/pch.cpp) 
    target_link_libraries(OVYoloCpp PUBLIC openvino::runtime Lumi)
    target_include_directories(OVYoloCpp PUBLIC source/win source/)
elseif(UNIX)
    # Shared Object
    add_library(OVYoloCpp SHARED source/ov_yolo.cc)
    target_compile_options(Lumi PUBLIC -fPIC)

    target_link_options(Lumi PUBLIC -fPIC)
    target_link_libraries(OVYoloCpp PUBLIC openvino::runtime Lumi)
endif()
