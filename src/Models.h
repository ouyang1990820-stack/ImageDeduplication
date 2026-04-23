#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

struct DirectoryEntry {
    int id = -1;
    QString path;
    QDateTime lastScanTime;
    bool recursive = true;
    QString status = QStringLiteral("normal");
};

struct ScanOptions {
    QStringList extensions;
    qint64 minimumSizeBytes = 0;
    qint64 maximumSizeBytes = 0;
    bool skipHiddenAndSystem = true;
    bool recursive = true;
    int maximumDepth = -1;
    QString algorithm = QStringLiteral("pHash");
    int similarityThreshold = 90;
};

struct ImageRecord {
    int id = -1;
    int directoryId = -1;
    QString directoryPath;
    QString filePath;
    QString fileName;
    qint64 fileSize = 0;
    int width = 0;
    int height = 0;
    QDateTime modifiedDate;
    QString md5Hash;
    QString sha256Hash;
    QByteArray perceptualHash;
    QString perceptualHashHex;
    QString hashAlgorithm;
    QString status = QStringLiteral("active");
    QDateTime scanTime;
};

struct SimilarMember {
    ImageRecord image;
    double similarity = 0.0;
    bool ignored = false;
};

struct SimilarGroupData {
    int groupId = -1;
    ImageRecord representative;
    QList<SimilarMember> members;
    double averageSimilarity = 0.0;
    qint64 totalSize = 0;
    bool exactGroup = false;
};
