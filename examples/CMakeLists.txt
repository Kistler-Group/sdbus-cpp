
add_executable(obj-manager-server org.freedesktop.DBus.ObjectManager/obj-manager-server.cpp)
target_link_libraries(obj-manager-server sdbus-c++)

add_executable(obj-manager-client org.freedesktop.DBus.ObjectManager/obj-manager-client.cpp)
target_link_libraries(obj-manager-client sdbus-c++)