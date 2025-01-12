cmake_minimum_required(VERSION 3.19)
project(HttpServer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core)

qt_standard_project_setup()

qt_add_executable(HttpServer
    src/main.cpp
    src/httpserver.h src/httpserver.cpp
    src/httpapi.h src/httpapi.cpp
    src/httprequest.h src/httprequest.cpp
    src/httpresponse.h src/httpresponse.cpp
)

find_package(Qt6
    REQUIRED COMPONENTS
        Network
)

target_link_libraries(HttpServer
    PRIVATE
        Qt::Core
        Qt6::Network
)

include(GNUInstallDirs)

install(TARGETS HttpServer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET HttpServer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
