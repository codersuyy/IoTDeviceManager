#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include "Device.h"
#include <vector>
#include <memory>
#include <string>

class FileStorage {
public:
    // Lưu toàn bộ danh sách thiết bị ra file.
    // Tham số là const vector<unique_ptr<Device>>& - tham chiếu (reference)
    // để KHÔNG copy (vì unique_ptr không cho copy) và KHÔNG sửa đổi (const).
    static bool save(const std::string& filename,
                      const std::vector<std::unique_ptr<Device>>& devices);

    // Đọc file, trả về 1 vector chứa các Device mới được tạo lại.
    // Trả vector rỗng nếu file không tồn tại hoặc lỗi.
    static std::vector<std::unique_ptr<Device>> load(const std::string& filename);

    // Factory function: dựa vào "typeName" (đọc từ file) để biết nên
    // "new" ra loại class con nào. Đây là cách giải quyết bài toán
    // "đọc dữ liệu generic, tạo lại object đúng loại cụ thể".
    static std::unique_ptr<Device> createDeviceByType(
        const std::string& typeName,
        const std::string& id,
        const std::string& name);
};

#endif // FILE_STORAGE_H