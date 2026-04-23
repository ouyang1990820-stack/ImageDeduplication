#pragma once

#include "DatabaseManager.h"
#include "Models.h"

#include <QFileInfo>
#include <QObject>

class ScannerService : public QObject {
    Q_OBJECT
public:
    explicit ScannerService(DatabaseManager* databaseManager, QObject* parent = nullptr);

    QList<ImageRecord> scan(const QStringList& directories, const ScanOptions& options);

signals:
    void logMessage(const QString& message);

private:
    void collectFiles(const QString& directoryPath, int currentDepth, const ScanOptions& options, QStringList& outFiles);
    bool shouldIncludeFile(const QFileInfo& fileInfo, const ScanOptions& options) const;
    DatabaseManager* databaseManager_ = nullptr;
};
