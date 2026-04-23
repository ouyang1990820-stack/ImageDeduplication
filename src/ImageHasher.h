#pragma once

#include <QByteArray>
#include <QImage>
#include <QString>

class ImageHasher {
public:
    static QByteArray computePerceptualHash(const QImage& image, const QString& algorithm);
    static int hammingDistance(const QByteArray& left, const QByteArray& right);
    static QString toHex(const QByteArray& hash);

private:
    static QByteArray computeAverageHash(const QImage& image);
    static QByteArray computeDifferenceHash(const QImage& image);
    static QByteArray computePerceptionHash(const QImage& image);
    static QByteArray packBits(const QVector<int>& bits);
};
