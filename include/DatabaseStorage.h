#ifndef DATABASE_STORAGE_H
#define DATABASE_STORAGE_H

#include "Device.h"
#include <string>
#include <vector>
#include <memory>

// Forward declare để không cần include sqlite3.h trong header —
// giữ cho header sạch, chỉ file .cpp mới cần biết chi tiết SQLite API.
struct sqlite3;

// Struct đơn giản để trả về lịch sử đọc — không cần class Device đầy đủ,
// chỉ cần các trường cần thiết để hiển thị/phân tích lịch sử.
struct DeviceReading {
    int         id;
    std::string deviceId;
    long long   timestamp;  // Unix time (giây từ 1/1/1970)
    std::string status;     // "ON" / "OFF"
    int         battery;
    std::string extra;      // serializeExtra() của từng loại thiết bị
};

class DatabaseStorage {
public:
    // Constructor: mở (hoặc tạo mới nếu chưa có) file DB tại đường dẫn cho trước.
    // SQLite tự tạo file nếu chưa tồn tại — không cần cài server riêng.
    explicit DatabaseStorage(const std::string& dbPath);

    // Destructor: đóng kết nối DB (RAII — resource tự được giải phóng khi
    // object ra khỏi scope, không cần gọi close() thủ công).
    ~DatabaseStorage();

    // Không cho phép copy DatabaseStorage (vì db_ là raw pointer sở hữu kết nối,
    // copy sẽ gây double-close -> crash).
    DatabaseStorage(const DatabaseStorage&)            = delete;
    DatabaseStorage& operator=(const DatabaseStorage&) = delete;

    // Tạo 2 bảng nếu chưa có — an toàn để gọi nhiều lần (IF NOT EXISTS).
    bool initSchema();

    // Lưu/cập nhật snapshot thiết bị (bảng devices).
    // INSERT OR REPLACE: nếu id đã tồn tại thì ghi đè, nếu chưa thì thêm mới.
    bool upsertDevice(const Device& device);

    // Thêm 1 dòng vào bảng readings (lịch sử) — gọi mỗi lần nhận message /data.
    bool insertReading(const Device& device);

    // Lấy N dòng lịch sử gần nhất của 1 thiết bị, sắp xếp mới nhất trước.
    std::vector<DeviceReading> getRecentReadings(const std::string& deviceId,
                                                  int limit = 10);

private:
    sqlite3*    db_;      // raw pointer vì SQLite C API dùng opaque pointer
    std::string dbPath_;

    // Helper: in lỗi SQLite ra stderr, trả về false để caller tự quyết
    // có tiếp tục hay không (không throw exception để đơn giản hóa).
    bool logError(const std::string& context) const;
};

#endif // DATABASE_STORAGE_H