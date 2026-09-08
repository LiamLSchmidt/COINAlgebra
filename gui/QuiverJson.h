#pragma once

#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Core/DecayVector.h"
#include <QByteArray>
#include <QJsonObject>
#include <memory>
#include <vector>

namespace Studio {
// Vectors refer to levels owned by this document's quiver.
struct Document {
    std::unique_ptr<DecayQuiver> quiver;
    std::vector<DecayVector> vectors;
    QJsonObject metadata;
    QString title;
};

// Throws std::runtime_error/invalid_argument for invalid input. Builds a new
// document so a failed import cannot modify an open workspace.
Document readJson(const QByteArray& bytes);
DecayVector createDecayVector(const DecayQuiver& quiver, const QJsonObject& metadata);
}
