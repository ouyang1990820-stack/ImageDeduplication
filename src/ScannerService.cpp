#include "ScannerService.h"

#include "ImageHasher.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>

ScannerService::ScannerService(DatabaseManager* databaseManager, QObject* parent)
    : QObject(parent), databaseManager_(databaseManager) {
}

QList<ImageRecord> ScannerService::scan(const QStringList& directories, const ScanOptions& options) {
    QList<ImageRecord> scannedImages;
    QStringList allFiles;
    for (const QString& directoryPath : directories) {
        if (!QFileInfo::exists(directoryPath)) {
            emit logMessage(tr("目录不存在：%1").arg(directoryPath));
            continue;
        }
        collectFiles(directoryPath, 0, options, allFiles);
    }

    emit logMessage(tr("准备扫描 %1 个文件").arg(allFiles.size()));
    for (const QString& filePath : std::as_const(allFiles)) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            emit logMessage(tr("无法读取文件：%1").arg(filePath));
            continue;
        }
        const QByteArray bytes = file.readAll();
        file.close();

        QImageReader reader(filePath);
        reader.setAutoTransform(true);
        const QImage image = reader.read();
        if (image.isNull()) {
            emit logMessage(tr("损坏或不支持的图片：%1").arg(filePath));
            continue;
        }

        QFileInfo info(filePath);
        const int directoryId = databaseManager_->upsertDirectory(info.absolutePath(), options.recursive);
        if (directoryId < 0) {
            emit logMessage(tr("目录记录失败：%1").arg(info.absolutePath()));
            continue;
        }

        ImageRecord record;
        record.directoryId = directoryId;
        record.directoryPath = info.absolutePath();
        record.filePath = info.absoluteFilePath();
        record.fileName = info.fileName();
        record.fileSize = info.size();
        record.width = image.width();
        record.height = image.height();
        record.modifiedDate = info.lastModified();
        record.md5Hash = QString::fromUtf8(QCryptographicHash::hash(bytes, QCryptographicHash::Md5).toHex());
        record.sha256Hash = QString::fromUtf8(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        record.hashAlgorithm = options.algorithm;
        record.scanTime = QDateTime::currentDateTimeUtc();

        QByteArray hashValue;
        if (!databaseManager_->findReusableHash(record.md5Hash, record.sha256Hash, options.algorithm, hashValue)) {
            hashValue = ImageHasher::computePerceptualHash(image, options.algorithm);
        }
        record.perceptualHash = hashValue;
        record.perceptualHashHex = ImageHasher::toHex(hashValue);

        if (!databaseManager_->upsertImage(record)) {
            emit logMessage(tr("图片记录失败：%1").arg(record.filePath));
            continue;
        }
        databaseManager_->updateDirectoryScanTime(directoryId, record.scanTime);
        scannedImages.append(record);
    }

    databaseManager_->markMissingRecords();
    emit logMessage(tr("扫描完成，处理 %1 张图片").arg(scannedImages.size()));
    return scannedImages;
}

void ScannerService::collectFiles(const QString& directoryPath, int currentDepth, const ScanOptions& options, QStringList& outFiles) {
    QDir dir(directoryPath);
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                                                    QDir::DirsFirst | QDir::Name);

    for (const QFileInfo& entry : entries) {
        if (entry.isDir()) {
            if (options.skipHiddenAndSystem && entry.isHidden()) {
                continue;
            }
            if (options.recursive && (options.maximumDepth < 0 || currentDepth < options.maximumDepth)) {
                collectFiles(entry.absoluteFilePath(), currentDepth + 1, options, outFiles);
            }
            continue;
        }
        if (shouldIncludeFile(entry, options)) {
            outFiles.append(entry.absoluteFilePath());
        }
    }
}

bool ScannerService::shouldIncludeFile(const QFileInfo& fileInfo, const ScanOptions& options) const {
    if (options.skipHiddenAndSystem && fileInfo.isHidden()) {
        return false;
    }
    const QString suffix = fileInfo.suffix().toLower();
    if (!options.extensions.isEmpty() && !options.extensions.contains(suffix)) {
        return false;
    }
    if (options.minimumSizeBytes > 0 && fileInfo.size() < options.minimumSizeBytes) {
        return false;
    }
    if (options.maximumSizeBytes > 0 && fileInfo.size() > options.maximumSizeBytes) {
        return false;
    }
    return true;
}
