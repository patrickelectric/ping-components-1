add_library(
    link
STATIC
    abstractlink.cpp
    filelink.cpp
    link.cpp
    linkconfiguration.cpp
    logsensorstruct.cpp
    ping1dsimulationlink.cpp
    ping360simulationlink.cpp
    processlog.cpp
    sensorinfo.cpp
    seriallink.cpp
    simulationlink.cpp
    tcplink.cpp
    udplink.cpp
    abstractlinknamespace.h
)

MESSAGE("POTATOOOOOO2 ${PING_CPP_INCLUDE_DIR} ${CMAKE_SOURCE_DIR} ${ping-cpp}")

target_link_libraries(
    link
PRIVATE
    ping-cpp
    Qt5::Core
    Qt5::Network
    Qt5::SerialPort
    Qt5::Gui
)
