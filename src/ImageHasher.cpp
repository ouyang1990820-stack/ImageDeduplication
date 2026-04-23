#include "ImageHasher.h"

#include <QColor>
#include <QtMath>
#include <QVector>

namespace {
QImage scaledGray(const QImage& image, const QSize& size) {
    return image.convertToFormat(QImage::Format_Grayscale8).scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

int pixelGray(const QImage& image, int x, int y) {
    return qGray(image.pixel(x, y));
}
}

QByteArray ImageHasher::computePerceptualHash(const QImage& image, const QString& algorithm) {
    if (algorithm.compare(QStringLiteral("aHash"), Qt::CaseInsensitive) == 0) {
        return computeAverageHash(image);
    }
    if (algorithm.compare(QStringLiteral("dHash"), Qt::CaseInsensitive) == 0) {
        return computeDifferenceHash(image);
    }
    return computePerceptionHash(image);
}

QString ImageHasher::toHex(const QByteArray& hash) {
    return QString::fromUtf8(hash.toHex());
}

int ImageHasher::hammingDistance(const QByteArray& left, const QByteArray& right) {
    if (left.size() != right.size()) {
        return std::numeric_limits<int>::max();
    }

    int distance = 0;
    for (int i = 0; i < left.size(); ++i) {
        distance += qPopulationCount(static_cast<quint8>(left[i] ^ right[i]));
    }
    return distance;
}

QByteArray ImageHasher::computeAverageHash(const QImage& image) {
    const QImage gray = scaledGray(image, QSize(8, 8));
    int total = 0;
    QVector<int> values;
    values.reserve(64);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const int value = pixelGray(gray, x, y);
            total += value;
            values.append(value);
        }
    }
    const int average = total / qMax(1, values.size());
    QVector<int> bits;
    bits.reserve(64);
    for (int value : values) {
        bits.append(value >= average ? 1 : 0);
    }
    return packBits(bits);
}

QByteArray ImageHasher::computeDifferenceHash(const QImage& image) {
    const QImage gray = scaledGray(image, QSize(9, 8));
    QVector<int> bits;
    bits.reserve(64);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            bits.append(pixelGray(gray, x, y) < pixelGray(gray, x + 1, y) ? 1 : 0);
        }
    }
    return packBits(bits);
}

QByteArray ImageHasher::computePerceptionHash(const QImage& image) {
    const QImage gray = scaledGray(image, QSize(32, 32));
    constexpr int size = 32;
    constexpr int reduced = 8;
    double dct[size][size];

    for (int u = 0; u < size; ++u) {
        for (int v = 0; v < size; ++v) {
            double sum = 0.0;
            for (int x = 0; x < size; ++x) {
                for (int y = 0; y < size; ++y) {
                    sum += pixelGray(gray, x, y)
                        * qCos((2.0 * x + 1.0) * u * M_PI / (2.0 * size))
                        * qCos((2.0 * y + 1.0) * v * M_PI / (2.0 * size));
                }
            }
            const double alphaU = (u == 0) ? qSqrt(1.0 / size) : qSqrt(2.0 / size);
            const double alphaV = (v == 0) ? qSqrt(1.0 / size) : qSqrt(2.0 / size);
            dct[u][v] = alphaU * alphaV * sum;
        }
    }

    QVector<double> coefficients;
    coefficients.reserve(reduced * reduced - 1);
    for (int u = 0; u < reduced; ++u) {
        for (int v = 0; v < reduced; ++v) {
            if (u == 0 && v == 0) {
                continue;
            }
            coefficients.append(dct[u][v]);
        }
    }

    QVector<double> sorted = coefficients;
    std::sort(sorted.begin(), sorted.end());
    const double median = sorted.at(sorted.size() / 2);

    QVector<int> bits;
    bits.reserve(64);
    for (double value : coefficients) {
        bits.append(value >= median ? 1 : 0);
    }
    while (bits.size() < 64) {
        bits.append(0);
    }
    return packBits(bits);
}

QByteArray ImageHasher::packBits(const QVector<int>& bits) {
    QByteArray bytes((bits.size() + 7) / 8, Qt::Uninitialized);
    bytes.fill(0);
    for (int i = 0; i < bits.size(); ++i) {
        if (bits[i]) {
            bytes[i / 8] = static_cast<char>(bytes[i / 8] | (1 << (7 - (i % 8))));
        }
    }
    return bytes;
}
