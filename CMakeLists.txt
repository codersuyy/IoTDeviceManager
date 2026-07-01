cmake_minimum_required(VERSION 3.10)
project(IoTDeviceManager CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(PahoMqttCpp REQUIRED)

add_executable(iot_manager
    src/Device.cpp
    src/TemperatureSensor.cpp
    src/SmartLight.cpp
    src/SecurityCamera.cpp
    src/DeviceManager.cpp
    src/FileStorage.cpp
    src/main.cpp
)

add_executable(manager_subscriber
    src/ManagerSubscriber.cpp
    src/Device.cpp
    src/TemperatureSensor.cpp
    src/SmartLight.cpp
    src/SecurityCamera.cpp
    src/DeviceManager.cpp
    src/FileStorage.cpp
)
target_link_libraries(manager_subscriber PahoMqttCpp::paho-mqttpp3)

add_executable(device_publisher
    src/DevicePublisher.cpp
    src/Device.cpp
    src/TemperatureSensor.cpp
    src/SmartLight.cpp
    src/SecurityCamera.cpp
)
target_link_libraries(device_publisher PahoMqttCpp::paho-mqttpp3)