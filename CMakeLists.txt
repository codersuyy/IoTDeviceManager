cmake_minimum_required(VERSION 3.10)
project(IoTDeviceManager CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Khai báo include directory - đây chính là thứ thay thế cho việc
# phải viết "../include/X.h" trong code. Mọi file .cpp trong target
# bên dưới sẽ tự động biết tìm header trong thư mục include/.
include_directories(${CMAKE_SOURCE_DIR}/include)

# --- MQTT (Paho C++) ---
# Trên macOS qua Homebrew, paho-mqttpp cài vào /opt/homebrew, CMake
# thường tự tìm thấy nhờ Homebrew set PKG_CONFIG/CMAKE prefix path sẵn.
# Nếu cmake báo "could not find PahoMqttCpp", thêm dòng:
#   set(CMAKE_PREFIX_PATH "/opt/homebrew")
find_package(PahoMqttCpp REQUIRED)

# Liệt kê rõ từng file .cpp thay vì dùng GLOB.
# Ưu điểm: CMake luôn biết chính xác danh sách file ngay khi đọc
# CMakeLists.txt, không bị "cache cũ" khi thêm file mới (như GLOB gặp phải).
# Nhược điểm: phải tự thêm tên file mới vào đây mỗi khi tạo thêm 1 file .cpp.
add_executable(iot_manager
    src/Device.cpp
    src/TemperatureSensor.cpp
    src/SmartLight.cpp
    src/SecurityCamera.cpp
    src/DeviceManager.cpp
    src/FileStorage.cpp
    src/main.cpp
)

# Liên kết thư viện Paho MQTT C++ với target iot_manager
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