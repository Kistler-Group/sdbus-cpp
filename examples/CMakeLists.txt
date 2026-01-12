# Building examples

set(OBJECTMANAGER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/org.freedesktop.DBus.ObjectManager)
set(OBJECTMANAGER_GENERATED_DIR ${OBJECTMANAGER_SOURCE_DIR}/dbus-api/gen-cpp)
set(OBJECTMANAGER_SERVER_SRCS
    ${OBJECTMANAGER_SOURCE_DIR}/obj-manager-server.cpp
    ${OBJECTMANAGER_GENERATED_DIR}/examplemanager-planet1-server-glue.h)

add_executable(obj-manager-server ${OBJECTMANAGER_SERVER_SRCS})
target_include_directories(obj-manager-server SYSTEM PRIVATE ${OBJECTMANAGER_GENERATED_DIR})
target_link_libraries(obj-manager-server sdbus-c++)

set(OBJECTMANAGER_CLIENT_SRCS
    ${OBJECTMANAGER_SOURCE_DIR}/obj-manager-client.cpp
    ${OBJECTMANAGER_GENERATED_DIR}/examplemanager-planet1-client-glue.h)

add_executable(obj-manager-client ${OBJECTMANAGER_CLIENT_SRCS})
target_include_directories(obj-manager-client SYSTEM PRIVATE ${OBJECTMANAGER_GENERATED_DIR})
target_link_libraries(obj-manager-client sdbus-c++)

if(SDBUSCPP_INSTALL)
    install(TARGETS obj-manager-server DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT sdbus-c++-examples)
    install(TARGETS obj-manager-client DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT sdbus-c++-examples)
endif()
