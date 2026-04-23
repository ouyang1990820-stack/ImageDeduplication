#include "SimilarityService.h"

#include "ImageHasher.h"

#include <QHash>
#include <numeric>

namespace {
class DisjointSet {
public:
    explicit DisjointSet(int count) : parent(count), rank(count, 0) {
        std::iota(parent.begin(), parent.end(), 0);
    }

    int find(int value) {
        if (parent[value] != value) {
            parent[value] = find(parent[value]);
        }
        return parent[value];
    }

    void unite(int left, int right) {
        left = find(left);
        right = find(right);
        if (left == right) {
            return;
        }
        if (rank[left] < rank[right]) {
            std::swap(left, right);
        }
        parent[right] = left;
        if (rank[left] == rank[right]) {
            ++rank[left];
        }
    }

private:
    QVector<int> parent;
    QVector<int> rank;
};

SimilarGroupData finalizeGroup(const QList<ImageRecord>& images, const QList<double>& similarities, bool exactGroup) {
    SimilarGroupData group;
    group.exactGroup = exactGroup;
    double totalSimilarity = 0.0;
    qint64 totalSize = 0;

    int representativeIndex = 0;
    for (int i = 0; i < images.size(); ++i) {
        if (images[i].fileSize > images[representativeIndex].fileSize) {
            representativeIndex = i;
        }
    }
    group.representative = images[representativeIndex];

    for (int i = 0; i < images.size(); ++i) {
        SimilarMember member;
        member.image = images[i];
        member.similarity = similarities.value(i, exactGroup ? 100.0 : 0.0);
        group.members.append(member);
        totalSimilarity += member.similarity;
        totalSize += member.image.fileSize;
    }
    group.averageSimilarity = group.members.isEmpty() ? 0.0 : totalSimilarity / group.members.size();
    group.totalSize = totalSize;
    return group;
}
}

QList<SimilarGroupData> SimilarityService::buildGroups(const QList<ImageRecord>& images, int similarityThreshold) const {
    QList<SimilarGroupData> groups = buildExactGroups(images);

    QSet<QString> exactPaths;
    for (const auto& group : groups) {
        for (const auto& member : group.members) {
            exactPaths.insert(member.image.filePath);
        }
    }

    QList<ImageRecord> remaining;
    for (const auto& image : images) {
        if (!exactPaths.contains(image.filePath)) {
            remaining.append(image);
        }
    }

    const auto fuzzyGroups = buildFuzzyGroups(remaining, similarityThreshold);
    groups.append(fuzzyGroups);
    return groups;
}

QList<SimilarGroupData> SimilarityService::buildExactGroups(const QList<ImageRecord>& images) const {
    QHash<QString, QList<ImageRecord>> byDigest;
    for (const auto& image : images) {
        byDigest[image.md5Hash + image.sha256Hash].append(image);
    }

    QList<SimilarGroupData> groups;
    for (auto it = byDigest.cbegin(); it != byDigest.cend(); ++it) {
        if (it.value().size() < 2) {
            continue;
        }
        QList<double> similarities(it.value().size(), 100.0);
        groups.append(finalizeGroup(it.value(), similarities, true));
    }
    return groups;
}

QList<SimilarGroupData> SimilarityService::buildFuzzyGroups(const QList<ImageRecord>& images, int similarityThreshold) const {
    QList<SimilarGroupData> groups;
    if (images.size() < 2) {
        return groups;
    }

    DisjointSet set(images.size());
    constexpr int bitCount = 64;
    for (int i = 0; i < images.size(); ++i) {
        for (int j = i + 1; j < images.size(); ++j) {
            const int distance = ImageHasher::hammingDistance(images[i].perceptualHash, images[j].perceptualHash);
            if (distance == std::numeric_limits<int>::max()) {
                continue;
            }
            const double similarity = 100.0 * (bitCount - distance) / bitCount;
            if (similarity >= similarityThreshold) {
                set.unite(i, j);
            }
        }
    }

    QHash<int, QList<int>> clusters;
    for (int i = 0; i < images.size(); ++i) {
        clusters[set.find(i)].append(i);
    }

    for (auto it = clusters.cbegin(); it != clusters.cend(); ++it) {
        if (it.value().size() < 2) {
            continue;
        }
        QList<ImageRecord> groupImages;
        QList<double> similarities;
        int representativeIndex = it.value().first();
        for (int index : it.value()) {
            if (images[index].fileSize > images[representativeIndex].fileSize) {
                representativeIndex = index;
            }
        }
        for (int index : it.value()) {
            groupImages.append(images[index]);
            const int distance = ImageHasher::hammingDistance(images[representativeIndex].perceptualHash, images[index].perceptualHash);
            similarities.append(100.0 * (bitCount - distance) / bitCount);
        }
        groups.append(finalizeGroup(groupImages, similarities, false));
    }
    return groups;
}
