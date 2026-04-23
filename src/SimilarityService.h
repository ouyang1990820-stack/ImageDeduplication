#pragma once

#include "Models.h"

class SimilarityService {
public:
    QList<SimilarGroupData> buildGroups(const QList<ImageRecord>& images, int similarityThreshold) const;

private:
    QList<SimilarGroupData> buildExactGroups(const QList<ImageRecord>& images) const;
    QList<SimilarGroupData> buildFuzzyGroups(const QList<ImageRecord>& images, int similarityThreshold) const;
};
