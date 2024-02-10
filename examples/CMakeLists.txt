# Building examples

add_executable(obj-manager-server org.freedesktop.DBus.ObjectManager/obj-manager-server.cpp)
target_link_libraries(obj-manager-server sdbus-c++)

add_executable(obj-manager-client org.freedesktop.DBus.ObjectManager/obj-manager-client.cpp)
target_link_libraries(obj-manager-client sdbus-c++)

if(SDBUSCPP_INSTALL)
    install(TARGETS obj-manager-server DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT examples)
    install(TARGETS obj-manager-client DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT examples)
endif()
