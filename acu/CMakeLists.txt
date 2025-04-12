add_executable(acu_controller
    src/main.cpp
    src/sensors/SensorManager.cpp
    src/sensors/MPU6050.cpp
    src/sensors/GPS.cpp
    src/sensors/Ultrasonic.cpp
    src/control/FlightController.cpp
    src/control/PWMController.cpp
    src/communication/CommunicationManager.cpp
    src/state/StateMachine.cpp
)

target_include_directories(acu_controller PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common/include
)

target_link_libraries(acu_controller PRIVATE
    common
    Threads::Threads
    Boost::system
    wiringPi
    i2c
)

# Installation
install(TARGETS acu_controller
    RUNTIME DESTINATION bin
)

# Configuration files
install(FILES config/acu_config.yaml
    DESTINATION etc/mm-drone
) 