#pragma once

#include "Models.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QSqlDatabase>

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);

    bool initialize(const QString& databasePath);
    QString databasePath() const;
    QString lastError() const;

    int upsertDirectory(const QString& path, bool recursive, const QString& status = QStringLiteral("normal"));
    bool updateDirectoryScanTime(int directoryId, const QDateTime& scanTime);
    bool upsertImage(const ImageRecord& image);
    bool saveGroups(const QList<SimilarGroupData>& groups);
    bool removeDirectory(const QString& path);
    bool clearDirectories();

    QList<DirectoryEntry> loadDirectories() const;
    QList<ImageRecord> loadActiveImages(const QString& algorithm) const;
    QList<SimilarGroupData> loadGroups(const QString& algorithm) const;

    bool findReusableHash(const QString& md5Hash, const QString& sha256Hash, const QString& algorithm, QByteArray& outHash) const;
    bool markMissingRecords();
    bool cleanupMissingRecords();

private:
    bool ensureSchema();
    QSqlQuery makeQuery() const;
    QString connectionName_;
    QString databasePath_;
    QString lastError_;
};
