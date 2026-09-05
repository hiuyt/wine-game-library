cmake_minimum_required(VERSION 3.20)
project(SystemWineLauncher VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(system-wine-launcher src/main.cpp)
target_link_libraries(system-wine-launcher PRIVATE Qt6::Widgets)
qt_add_resources(system-wine-launcher app_icons
    PREFIX "/icons"
    FILES assets/wine-game-library.png)
install(TARGETS system-wine-launcher RUNTIME DESTINATION bin)
install(FILES packaging/system-wine-launcher.desktop DESTINATION share/applications)
foreach(size 64 128 256 512)
    install(FILES packaging/icons/${size}x${size}/wine-game-library.png
            DESTINATION share/icons/hicolor/${size}x${size}/apps)
endforeach()
