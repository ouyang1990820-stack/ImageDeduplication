cmake_minimum_required(VERSION 3.21)
project(ImageDeduplication VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

qt_add_executable(ImageDeduplication
    WIN32
    src/main.cpp
    src/Models.h
    src/DatabaseManager.h
    src/DatabaseManager.cpp
    src/ImageHasher.h
    src/ImageHasher.cpp
    src/ScannerService.h
    src/ScannerService.cpp
    src/SimilarityService.h
    src/SimilarityService.cpp
    src/PreviewDialog.h
    src/PreviewDialog.cpp
    src/DeleteConfirmationDialog.h
    src/DeleteConfirmationDialog.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(ImageDeduplication PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
)

install(TARGETS ImageDeduplication
    RUNTIME DESTINATION .
)
