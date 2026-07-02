#include "DatabaseStorage.h"
#include <sqlite3.h>   // SQLite C API — chỉ include ở .cpp, không để lộ ra header
#include <iostream>
#include <chrono>      // std::chrono::system_clock để lấy timestamp hiện tại

// ─── Constructor / Destructor ────────────────────────────────────────────────

DatabaseStorage::DatabaseStorage(const std::string& dbPath)
    : db_(nullptr), dbPath_(dbPath) {

    // sqlite3_open: mở file DB. Nếu file chưa tồn tại, SQLite tự tạo mới.
    // &db_ truyền địa chỉ của con trỏ để hàm C gán giá trị vào (C-style output param).
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Khong the mo DB '" << dbPath << "': "
                  << sqlite3_errmsg(db_) << "\n";
        sqlite3_close(db_); // phải đóng dù mở lỗi để tránh leak
        db_ = nullptr;
    } else {
        std::cout << "[DB] Da mo DB: " << dbPath << "\n";
    }
}

DatabaseStorage::~DatabaseStorage() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

// ─── Helper ──────────────────────────────────────────────────────────────────

bool DatabaseStorage::logError(const std::string& context) const {
    std::cerr << "[DB] Loi " << context << ": "
              << (db_ ? sqlite3_errmsg(db_) : "db_ = null") << "\n";
    return false;
}

// ─── Schema ──────────────────────────────────────────────────────────────────

bool DatabaseStorage::initSchema() {
    if (!db_) return false;

    // 1 chuỗi SQL chứa cả 2 lệnh CREATE TABLE — SQLite cho phép chạy nhiều
    // lệnh cùng lúc qua sqlite3_exec (tách bằng dấu ;).
    // "IF NOT EXISTS" đảm bảo an toàn khi gọi lại (idempotent).
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS devices (
            id      TEXT PRIMARY KEY,
            type    TEXT NOT NULL,
            name    TEXT NOT NULL,
            status  TEXT NOT NULL,
            battery INTEGER NOT NULL,
            extra   TEXT
        );

        CREATE TABLE IF NOT EXISTS readings (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            status    TEXT NOT NULL,
            battery   INTEGER NOT NULL,
            extra     TEXT,
            FOREIGN KEY (device_id) REFERENCES devices(id)
        );
    )";

    // sqlite3_exec: chạy SQL không cần kết quả trả về (DDL, INSERT, UPDATE...).
    // Tham số callback (thứ 3) và errmsg (thứ 4) để nhận lỗi — ở đây dùng
    // nullptr cho callback (không cần xử lý kết quả), dùng &errMsg cho lỗi.
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Loi tao schema: " << errMsg << "\n";
        sqlite3_free(errMsg); // phải giải phóng chuỗi lỗi do SQLite cấp phát
        return false;
    }

    std::cout << "[DB] Schema san sang.\n";
    return true;
}

// ─── Upsert snapshot thiết bị ────────────────────────────────────────────────

bool DatabaseStorage::upsertDevice(const Device& device) {
    if (!db_) return false;

    // INSERT OR REPLACE: nếu id đã tồn tại thì xóa dòng cũ và insert dòng mới
    // (giống UPDATE nhưng đơn giản hơn khi cần ghi đè toàn bộ cột).
    const char* sql =
        "INSERT OR REPLACE INTO devices (id, type, name, status, battery, extra) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    // sqlite3_stmt: "prepared statement" — SQL được parse 1 lần, rồi bind
    // tham số vào sau. An toàn hơn string concatenation (tránh SQL injection),
    // và hiệu quả hơn nếu gọi nhiều lần.
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return logError("prepare upsertDevice");

    // Bind tham số: vị trí đánh số từ 1 (không phải 0).
    // SQLITE_TRANSIENT: SQLite tự copy chuỗi -> an toàn dù biến C++ bị xóa
    // sau khi hàm này kết thúc.
    sqlite3_bind_text(stmt, 1, device.getId().c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, device.getTypeName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, device.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, device.isOn() ? "ON" : "OFF", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 5, device.getBatteryLevel());
    sqlite3_bind_text(stmt, 6, device.serializeExtra().c_str(), -1, SQLITE_TRANSIENT);

    // sqlite3_step: thực thi 1 lần — với INSERT/UPDATE/DELETE kết quả mong đợi
    // là SQLITE_DONE (không phải SQLITE_ROW vì không có dữ liệu trả về).
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) logError("step upsertDevice");

    // Luôn phải finalize để giải phóng tài nguyên của prepared statement,
    // dù step thành công hay thất bại.
    sqlite3_finalize(stmt);
    return ok;
}

// ─── Insert lịch sử ──────────────────────────────────────────────────────────

bool DatabaseStorage::insertReading(const Device& device) {
    if (!db_) return false;

    const char* sql =
        "INSERT INTO readings (device_id, timestamp, status, battery, extra) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return logError("prepare insertReading");

    // Lấy timestamp hiện tại (giây từ Unix epoch) — dùng std::chrono để
    // đảm bảo cross-platform (không phụ thuộc vào time() của C).
    auto now = std::chrono::system_clock::now();
    long long ts = std::chrono::duration_cast<std::chrono::seconds>(
                       now.time_since_epoch()).count();

    sqlite3_bind_text(stmt, 1, device.getId().c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, ts);
    sqlite3_bind_text(stmt, 3, device.isOn() ? "ON" : "OFF", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 4, device.getBatteryLevel());
    sqlite3_bind_text(stmt, 5, device.serializeExtra().c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) logError("step insertReading");

    sqlite3_finalize(stmt);
    return ok;
}

// ─── Query lịch sử ───────────────────────────────────────────────────────────

std::vector<DeviceReading> DatabaseStorage::getRecentReadings(
    const std::string& deviceId, int limit) {

    std::vector<DeviceReading> result;
    if (!db_) return result;

    const char* sql =
        "SELECT id, device_id, timestamp, status, battery, extra "
        "FROM readings "
        "WHERE device_id = ? "
        "ORDER BY timestamp DESC "  // mới nhất trước
        "LIMIT ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        logError("prepare getRecentReadings");
        return result;
    }

    sqlite3_bind_text(stmt, 1, deviceId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 2, limit);

    // sqlite3_step trả về SQLITE_ROW khi còn dòng dữ liệu để đọc,
    // SQLITE_DONE khi hết — khác với INSERT/UPDATE chỉ gọi 1 lần.
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DeviceReading r;
        r.id        = sqlite3_column_int  (stmt, 0);
        r.deviceId  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        r.timestamp = sqlite3_column_int64(stmt, 2);
        r.status    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.battery   = sqlite3_column_int  (stmt, 4);

        // sqlite3_column_text trả về nullptr nếu cột là NULL trong DB
        const unsigned char* extra = sqlite3_column_text(stmt, 5);
        r.extra = extra ? reinterpret_cast<const char*>(extra) : "";

        result.push_back(r);
    }

    sqlite3_finalize(stmt);
    return result;
}