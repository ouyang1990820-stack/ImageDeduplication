#include "DatabaseManager.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent), connectionName_(QUuid::createUuid().toString(QUuid::WithoutBraces)) {
}

bool DatabaseManager::initialize(const QString& databasePath) {
    databasePath_ = databasePath;
    QDir().mkpath(QFileInfo(databasePath).absolutePath());

    if (QSqlDatabase::contains(connectionName_)) {
        QSqlDatabase::removeDatabase(connectionName_);
    }

    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    database.setDatabaseName(databasePath_);
    if (!database.open()) {
        lastError_ = database.lastError().text();
        return false;
    }
    return ensureSchema();
}

QString DatabaseManager::databasePath() const {
    return databasePath_;
}

QString DatabaseManager::lastError() const {
    return lastError_;
}

QSqlQuery DatabaseManager::makeQuery() const {
    return QSqlQuery(QSqlDatabase::database(connectionName_));
}

bool DatabaseManager::ensureSchema() {
    const QStringList statements = {
        QStringLiteral("PRAGMA foreign_keys = ON"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS directories ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "path TEXT NOT NULL UNIQUE,"
                       "last_scan_time TEXT,"
                       "scan_recursive INTEGER,"
                       "status TEXT DEFAULT 'normal')"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS images ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "directory_id INTEGER,"
                       "file_path TEXT NOT NULL UNIQUE,"
                       "file_name TEXT,"
                       "file_size INTEGER,"
                       "resolution_width INTEGER,"
                       "resolution_height INTEGER,"
                       "modified_date TEXT,"
                       "md5_hash TEXT,"
                       "sha256_hash TEXT,"
                       "perceptual_hash TEXT,"
                       "hash_algorithm TEXT,"
                       "status TEXT DEFAULT 'active',"
                       "scan_time TEXT,"
                       "FOREIGN KEY(directory_id) REFERENCES directories(id))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS similar_groups ("
                       "group_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "representative_image_id INTEGER,"
                       "average_similarity REAL,"
                       "image_count INTEGER,"
                       "hash_algorithm TEXT,"
                       "exact_group INTEGER DEFAULT 0,"
                       "FOREIGN KEY(representative_image_id) REFERENCES images(id))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS group_members ("
                       "group_id INTEGER,"
                       "image_id INTEGER,"
                       "similarity_score REAL,"
                       "ignored INTEGER DEFAULT 0,"
                       "PRIMARY KEY(group_id, image_id),"
                       "FOREIGN KEY(group_id) REFERENCES similar_groups(group_id),"
                       "FOREIGN KEY(image_id) REFERENCES images(id))"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_images_md5_sha ON images(md5_hash, sha256_hash, hash_algorithm)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_group_members_group ON group_members(group_id)")
    };

    for (const QString& statement : statements) {
        auto query = makeQuery();
        if (!query.exec(statement)) {
            lastError_ = query.lastError().text();
            return false;
        }
    }
    return true;
}

int DatabaseManager::upsertDirectory(const QString& path, bool recursive, const QString& status) {
    auto query = makeQuery();
    query.prepare(QStringLiteral(
        "INSERT INTO directories(path, last_scan_time, scan_recursive, status) VALUES(?, ?, ?, ?) "
        "ON CONFLICT(path) DO UPDATE SET scan_recursive=excluded.scan_recursive, status=excluded.status"));
    query.addBindValue(path);
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(recursive ? 1 : 0);
    query.addBindValue(status);
    if (!query.exec()) {
        lastError_ = query.lastError().text();
        return -1;
    }

    auto selectQuery = makeQuery();
    selectQuery.prepare(QStringLiteral("SELECT id FROM directories WHERE path = ?"));
    selectQuery.addBindValue(path);
    if (!selectQuery.exec() || !selectQuery.next()) {
        lastError_ = selectQuery.lastError().text();
        return -1;
    }
    return selectQuery.value(0).toInt();
}

bool DatabaseManager::updateDirectoryScanTime(int directoryId, const QDateTime& scanTime) {
    auto query = makeQuery();
    query.prepare(QStringLiteral("UPDATE directories SET last_scan_time = ?, status = 'normal' WHERE id = ?"));
    query.addBindValue(scanTime.toString(Qt::ISODate));
    query.addBindValue(directoryId);
    if (!query.exec()) {
        lastError_ = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::upsertImage(const ImageRecord& image) {
    auto query = makeQuery();
    query.prepare(QStringLiteral(
        "INSERT INTO images(directory_id, file_path, file_name, file_size, resolution_width, resolution_height, modified_date, md5_hash, sha256_hash, perceptual_hash, hash_algorithm, status, scan_time) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(file_path) DO UPDATE SET directory_id=excluded.directory_id, file_name=excluded.file_name, file_size=excluded.file_size, resolution_width=excluded.resolution_width, resolution_height=excluded.resolution_height, modified_date=excluded.modified_date, md5_hash=excluded.md5_hash, sha256_hash=excluded.sha256_hash, perceptual_hash=excluded.perceptual_hash, hash_algorithm=excluded.hash_algorithm, status=excluded.status, scan_time=excluded.scan_time"));
    query.addBindValue(image.directoryId);
    query.addBindValue(image.filePath);
    query.addBindValue(image.fileName);
    query.addBindValue(image.fileSize);
    query.addBindValue(image.width);
    query.addBindValue(image.height);
    query.addBindValue(image.modifiedDate.toString(Qt::ISODate));
    query.addBindValue(image.md5Hash);
    query.addBindValue(image.sha256Hash);
    query.addBindValue(image.perceptualHashHex);
    query.addBindValue(image.hashAlgorithm);
    query.addBindValue(image.status);
    query.addBindValue(image.scanTime.toString(Qt::ISODate));
    if (!query.exec()) {
        lastError_ = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removeDirectory(const QString& path) {
    auto findQuery = makeQuery();
    findQuery.prepare(QStringLiteral("SELECT id FROM directories WHERE path = ?"));
    findQuery.addBindValue(path);
    if (!findQuery.exec()) {
        lastError_ = findQuery.lastError().text();
        return false;
    }
    if (!findQuery.next()) {
        return true;
    }

    const int directoryId = findQuery.value(0).toInt();
    auto database = QSqlDatabase::database(connectionName_);
    if (!database.transaction()) {
        lastError_ = database.lastError().text();
        return false;
    }

    auto deleteImages = makeQuery();
    deleteImages.prepare(QStringLiteral("DELETE FROM images WHERE directory_id = ?"));
    deleteImages.addBindValue(directoryId);
    if (!deleteImages.exec()) {
        lastError_ = deleteImages.lastError().text();
        database.rollback();
        return false;
    }

    auto deleteDirectory = makeQuery();
    deleteDirectory.prepare(QStringLiteral("DELETE FROM directories WHERE id = ?"));
    deleteDirectory.addBindValue(directoryId);
    if (!deleteDirectory.exec()) {
        lastError_ = deleteDirectory.lastError().text();
        database.rollback();
        return false;
    }

    return database.commit();
}

bool DatabaseManager::clearDirectories() {
    auto database = QSqlDatabase::database(connectionName_);
    if (!database.transaction()) {
        lastError_ = database.lastError().text();
        return false;
    }

    const QStringList statements = {
        QStringLiteral("DELETE FROM group_members"),
        QStringLiteral("DELETE FROM similar_groups"),
        QStringLiteral("DELETE FROM images"),
        QStringLiteral("DELETE FROM directories")
    };
    for (const QString& statement : statements) {
        auto query = makeQuery();
        if (!query.exec(statement)) {
            lastError_ = query.lastError().text();
            database.rollback();
            return false;
        }
    }

    return database.commit();
}

QList<DirectoryEntry> DatabaseManager::loadDirectories() const {
    QList<DirectoryEntry> directories;
    auto query = makeQuery();
    query.prepare(QStringLiteral("SELECT id, path, last_scan_time, scan_recursive, status FROM directories ORDER BY path"));
    if (!query.exec()) {
        return directories;
    }
    while (query.next()) {
        DirectoryEntry entry;
        entry.id = query.value(0).toInt();
        entry.path = query.value(1).toString();
        entry.lastScanTime = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
        entry.recursive = query.value(3).toInt() != 0;
        entry.status = query.value(4).toString();
        directories.append(entry);
    }
    return directories;
}

QList<ImageRecord> DatabaseManager::loadActiveImages(const QString& algorithm) const {
    QList<ImageRecord> images;
    auto query = makeQuery();
    query.prepare(QStringLiteral(
        "SELECT i.id, i.directory_id, d.path, i.file_path, i.file_name, i.file_size, i.resolution_width, i.resolution_height, i.modified_date, i.md5_hash, i.sha256_hash, i.perceptual_hash, i.hash_algorithm, i.status, i.scan_time "
        "FROM images i LEFT JOIN directories d ON d.id = i.directory_id "
        "WHERE i.status = 'active' AND i.hash_algorithm = ? ORDER BY i.file_path"));
    query.addBindValue(algorithm);
    if (!query.exec()) {
        return images;
    }
    while (query.next()) {
        ImageRecord image;
        image.id = query.value(0).toInt();
        image.directoryId = query.value(1).toInt();
        image.directoryPath = query.value(2).toString();
        image.filePath = query.value(3).toString();
        image.fileName = query.value(4).toString();
        image.fileSize = query.value(5).toLongLong();
        image.width = query.value(6).toInt();
        image.height = query.value(7).toInt();
        image.modifiedDate = QDateTime::fromString(query.value(8).toString(), Qt::ISODate);
        image.md5Hash = query.value(9).toString();
        image.sha256Hash = query.value(10).toString();
        image.perceptualHashHex = query.value(11).toString();
        image.perceptualHash = QByteArray::fromHex(image.perceptualHashHex.toUtf8());
        image.hashAlgorithm = query.value(12).toString();
        image.status = query.value(13).toString();
        image.scanTime = QDateTime::fromString(query.value(14).toString(), Qt::ISODate);
        images.append(image);
    }
    return images;
}

bool DatabaseManager::saveGroups(const QList<SimilarGroupData>& groups) {
    auto database = QSqlDatabase::database(connectionName_);
    if (!database.transaction()) {
        lastError_ = database.lastError().text();
        return false;
    }

    auto clearMembers = makeQuery();
    if (!clearMembers.exec(QStringLiteral("DELETE FROM group_members"))) {
        lastError_ = clearMembers.lastError().text();
        database.rollback();
        return false;
    }

    auto clearGroups = makeQuery();
    if (!clearGroups.exec(QStringLiteral("DELETE FROM similar_groups"))) {
        lastError_ = clearGroups.lastError().text();
        database.rollback();
        return false;
    }

    for (const auto& group : groups) {
        auto insertGroup = makeQuery();
        insertGroup.prepare(QStringLiteral(
            "INSERT INTO similar_groups(representative_image_id, average_similarity, image_count, hash_algorithm, exact_group) VALUES(?, ?, ?, ?, ?)"));
        insertGroup.addBindValue(group.representative.id);
        insertGroup.addBindValue(group.averageSimilarity);
        insertGroup.addBindValue(group.members.size());
        insertGroup.addBindValue(group.representative.hashAlgorithm);
        insertGroup.addBindValue(group.exactGroup ? 1 : 0);
        if (!insertGroup.exec()) {
            lastError_ = insertGroup.lastError().text();
            database.rollback();
            return false;
        }
        const int groupId = insertGroup.lastInsertId().toInt();

        for (const auto& member : group.members) {
            auto insertMember = makeQuery();
            insertMember.prepare(QStringLiteral(
                "INSERT INTO group_members(group_id, image_id, similarity_score, ignored) VALUES(?, ?, ?, ?)"));
            insertMember.addBindValue(groupId);
            insertMember.addBindValue(member.image.id);
            insertMember.addBindValue(member.similarity);
            insertMember.addBindValue(member.ignored ? 1 : 0);
            if (!insertMember.exec()) {
                lastError_ = insertMember.lastError().text();
                database.rollback();
                return false;
            }
        }
    }

    if (!database.commit()) {
        lastError_ = database.lastError().text();
        database.rollback();
        return false;
    }
    return true;
}

QList<SimilarGroupData> DatabaseManager::loadGroups(const QString& algorithm) const {
    QMap<int, SimilarGroupData> groups;
    auto query = makeQuery();
    query.prepare(QStringLiteral(
        "SELECT g.group_id, g.average_similarity, g.exact_group, gm.similarity_score, gm.ignored, "
        "i.id, i.directory_id, d.path, i.file_path, i.file_name, i.file_size, i.resolution_width, i.resolution_height, i.modified_date, i.md5_hash, i.sha256_hash, i.perceptual_hash, i.hash_algorithm, i.status, i.scan_time "
        "FROM similar_groups g "
        "JOIN group_members gm ON gm.group_id = g.group_id "
        "JOIN images i ON i.id = gm.image_id "
        "LEFT JOIN directories d ON d.id = i.directory_id "
        "WHERE g.hash_algorithm = ? ORDER BY g.group_id, gm.similarity_score DESC, i.file_path"));
    query.addBindValue(algorithm);
    if (!query.exec()) {
        return groups.values();
    }
    while (query.next()) {
        const int groupId = query.value(0).toInt();
        auto& group = groups[groupId];
        group.groupId = groupId;
        group.averageSimilarity = query.value(1).toDouble();
        group.exactGroup = query.value(2).toInt() != 0;

        ImageRecord image;
        image.id = query.value(5).toInt();
        image.directoryId = query.value(6).toInt();
        image.directoryPath = query.value(7).toString();
        image.filePath = query.value(8).toString();
        image.fileName = query.value(9).toString();
        image.fileSize = query.value(10).toLongLong();
        image.width = query.value(11).toInt();
        image.height = query.value(12).toInt();
        image.modifiedDate = QDateTime::fromString(query.value(13).toString(), Qt::ISODate);
        image.md5Hash = query.value(14).toString();
        image.sha256Hash = query.value(15).toString();
        image.perceptualHashHex = query.value(16).toString();
        image.perceptualHash = QByteArray::fromHex(image.perceptualHashHex.toUtf8());
        image.hashAlgorithm = query.value(17).toString();
        image.status = query.value(18).toString();
        image.scanTime = QDateTime::fromString(query.value(19).toString(), Qt::ISODate);

        SimilarMember member;
        member.image = image;
        member.similarity = query.value(3).toDouble();
        member.ignored = query.value(4).toInt() != 0;
        group.members.append(member);
        group.totalSize += image.fileSize;
        if (group.representative.id < 0 || image.fileSize > group.representative.fileSize) {
            group.representative = image;
        }
    }
    return groups.values();
}

bool DatabaseManager::findReusableHash(const QString& md5Hash, const QString& sha256Hash, const QString& algorithm, QByteArray& outHash) const {
    auto query = makeQuery();
    query.prepare(QStringLiteral(
        "SELECT perceptual_hash FROM images WHERE md5_hash = ? AND sha256_hash = ? AND hash_algorithm = ? AND status != 'missing' LIMIT 1"));
    query.addBindValue(md5Hash);
    query.addBindValue(sha256Hash);
    query.addBindValue(algorithm);
    if (!query.exec() || !query.next()) {
        return false;
    }
    outHash = QByteArray::fromHex(query.value(0).toString().toUtf8());
    return !outHash.isEmpty();
}

bool DatabaseManager::markMissingRecords() {
    auto selectQuery = makeQuery();
    if (!selectQuery.exec(QStringLiteral("SELECT id, file_path FROM images"))) {
        lastError_ = selectQuery.lastError().text();
        return false;
    }
    while (selectQuery.next()) {
        auto updateQuery = makeQuery();
        updateQuery.prepare(QStringLiteral("UPDATE images SET status = ? WHERE id = ?"));
        updateQuery.addBindValue(QFileInfo::exists(selectQuery.value(1).toString()) ? QStringLiteral("active") : QStringLiteral("missing"));
        updateQuery.addBindValue(selectQuery.value(0).toInt());
        if (!updateQuery.exec()) {
            lastError_ = updateQuery.lastError().text();
            return false;
        }
    }

    auto directoryQuery = makeQuery();
    if (!directoryQuery.exec(QStringLiteral("SELECT id, path FROM directories"))) {
        lastError_ = directoryQuery.lastError().text();
        return false;
    }
    while (directoryQuery.next()) {
        auto updateQuery = makeQuery();
        updateQuery.prepare(QStringLiteral("UPDATE directories SET status = ? WHERE id = ?"));
        updateQuery.addBindValue(QFileInfo::exists(directoryQuery.value(1).toString()) ? QStringLiteral("normal") : QStringLiteral("missing"));
        updateQuery.addBindValue(directoryQuery.value(0).toInt());
        if (!updateQuery.exec()) {
            lastError_ = updateQuery.lastError().text();
            return false;
        }
    }
    return true;
}

bool DatabaseManager::cleanupMissingRecords() {
    auto query = makeQuery();
    if (!query.exec(QStringLiteral("DELETE FROM images WHERE status = 'missing'"))) {
        lastError_ = query.lastError().text();
        return false;
    }
    auto directoryQuery = makeQuery();
    if (!directoryQuery.exec(QStringLiteral("DELETE FROM directories WHERE status = 'missing'"))) {
        lastError_ = directoryQuery.lastError().text();
        return false;
    }
    return true;
}
