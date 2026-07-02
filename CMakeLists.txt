cmake_minimum_required(VERSION 3.10)
project(IoTDeviceManager CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# --- MQTT (Paho C++) ---
set(CMAKE_PREFIX_PATH "/opt/homebrew")
find_package(PahoMqttCpp REQUIRED)

# --- SQLite3 ---
# Keg-only trên macOS: không được symlink vào /opt/homebrew vì macOS đã
# có sẵn SQLite cũ hơn. Phải chỉ đường dẫn rõ ràng tới bản Homebrew mới.
set(SQLITE3_INCLUDE_DIR "/opt/homebrew/opt/sqlite/include")
set(SQLITE3_LIB_DIR     "/opt/homebrew/opt/sqlite/lib")
include_directories(${SQLITE3_INCLUDE_DIR})

# Nguồn dùng chung cho TẤT CẢ target (không phụ thuộc DB hay MQTT)
set(CORE_SOURCES
    src/Device.cpp
    src/TemperatureSensor.cpp
    src/SmartLight.cpp
    src/SecurityCamera.cpp
    src/DeviceManager.cpp
    src/FileStorage.cpp
)

# iot_manager: test offline, không cần DB hay MQTT
add_executable(iot_manager ${CORE_SOURCES} src/main.cpp)

# manager_subscriber: cần cả MQTT và SQLite
add_executable(manager_subscriber
    ${CORE_SOURCES}
    src/DatabaseStorage.cpp
    src/ManagerSubscriber.cpp
)
target_link_libraries(manager_subscriber
    PahoMqttCpp::paho-mqttpp3
    -L${SQLITE3_LIB_DIR} -lsqlite3
)

add_executable(device_publisher
    src/DevicePublisher.cpp
    ${CORE_SOURCES}
)
target_link_libraries(device_publisher PahoMqttCpp::paho-mqttpp3)