cmake_minimum_required(VERSION 3.16)

project(yet-another-sign-up VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV found: ${OpenCV_VERSION}")
message(STATUS "Include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "Libraries: ${OpenCV_LIBS}")
include_directories(${OpenCV_INCLUDE_DIRS})



set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        signup.cpp signup.h
        adminpanel.h adminpanel.cpp adminpanel.ui
        arbiter.h arbiter.cpp arbiter.ui
        resourses.qrc
        LICENSE
)

# Load application icon
if(WIN32)
    set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico")

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc.in ${CMAKE_CURRENT_BINARY_DIR}/appicon.rc @ONLY)

    list(APPEND PROJECT_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/appicon.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(yet-another-sign-up
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET yet-another-sign-up APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(yet-another-sign-up SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(yet-another-sign-up
            ${PROJECT_SOURCES}
            LICENSE
            README.md
            favicon.ico
            resourses.qrc
        )
    endif()
endif()

target_link_libraries(yet-another-sign-up PRIVATE ${OpenCV_LIBS})
target_link_libraries(yet-another-sign-up PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.yet-another-sign-up)
endif()
set_target_properties(yet-another-sign-up PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS yet-another-sign-up
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(yet-another-sign-up)
endif()
