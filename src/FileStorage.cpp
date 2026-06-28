#include "FileStorage.h"
#include "TemperatureSensor.h"
#include "SmartLight.h"
#include "SecurityCamera.h"

#include <fstream>   // std::ofstream, std::ifstream
#include <sstream>   // std::ostringstream, std::istringstream
#include <iostream>

bool FileStorage::save(const std::string& filename,
                        const std::vector<std::unique_ptr<Device>>& devices) {
    // std::ofstream: "output file stream" - dùng để VIẾT ra file
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Loi: khong the mo file de luu: " << filename << "\n";
        return false;
    }

    // Dòng đầu: header, để file dễ đọc bằng mắt người (không bắt buộc,
    // nhưng là good practice khi tự định nghĩa file format)
    file << "TYPE,ID,NAME,STATUS,BATTERY,EXTRA\n";

    for (const auto& device : devices) {
        file << device->getTypeName() << ","
             << device->getId() << ","
             << device->getName() << ","
             << (device->isOn() ? "ON" : "OFF") << ","
             << device->getBatteryLevel() << ","
             << device->serializeExtra() << "\n";
    }

    // file tự động được "đóng" (flush + close) khi ra khỏi scope
    // (destructor của ofstream lo việc này), nhưng gọi rõ ràng cho dễ hiểu
    file.close();
    return true;
}
    
std::unique_ptr<Device> FileStorage::createDeviceByType(
    const std::string& typeName,
    const std::string& id,
    const std::string& name) {

    // Đây là "factory pattern" đơn giản bằng if-else chuỗi so sánh tên loại.
    // Khi thêm loại thiết bị mới (ví dụ SecurityCamera), chỉ cần thêm
    // 1 nhánh else if ở đây.
    if (typeName == "TemperatureSensor") {
        return std::make_unique<TemperatureSensor>(id, name);
    } else if (typeName == "SmartLight") {
        return std::make_unique<SmartLight>(id, name);
    } else if (typeName == "SecurityCamera") {
        return std::make_unique<SecurityCamera>(id, name);
    }

    // Loại không nhận diện được -> trả về nullptr, bên gọi sẽ tự bỏ qua dòng này
    std::cerr << "Canh bao: khong nhan dien duoc loai thiet bi '" 
               << typeName << "', bo qua dong nay.\n";
    return nullptr;
}

std::vector<std::unique_ptr<Device>> FileStorage::load(const std::string& filename) {
    std::vector<std::unique_ptr<Device>> devices;

    // std::ifstream: "input file stream" - dùng để ĐỌC từ file
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Khong tim thay file: " << filename 
                   << " (co the day la lan dau chay, chua co du lieu cu)\n";
        return devices; // trả về vector rỗng, không phải lỗi nghiêm trọng
    }

    std::string line;
    bool isFirstLine = true;

    // std::getline đọc từng dòng một, cho đến khi hết file
    while (std::getline(file, line)) {
        if (isFirstLine) {
            isFirstLine = false;
            continue; // bỏ qua dòng header "TYPE,ID,NAME,..."
        }

        if (line.empty()) continue; // bỏ qua dòng trống (nếu có)

        // Parse 1 dòng CSV: tách theo dấu ","
        // Lưu ý: cách này đơn giản, sẽ lỗi nếu NAME chứa dấu ",".
        // Với project học tập thì đủ dùng; project thật cần parser CSV
        // chuẩn hơn (xử lý dấu nháy, escape...).
        std::istringstream iss(line);
        std::string typeName, id, name, statusStr, batteryStr, extra;

        std::getline(iss, typeName, ',');
        std::getline(iss, id, ',');
        std::getline(iss, name, ',');
        std::getline(iss, statusStr, ',');
        std::getline(iss, batteryStr, ',');
        std::getline(iss, extra); // phần còn lại của dòng (có thể chứa ";")

        auto device = createDeviceByType(typeName, id, name);
        if (!device) continue; // không nhận diện được loại -> bỏ qua dòng này

        device->setStatus(statusStr == "ON" ? DeviceStatus::ON : DeviceStatus::OFF);
        device->setBatteryLevel(std::stoi(batteryStr));
        device->deserializeExtra(extra);

        devices.push_back(std::move(device));
    }

    return devices;
}