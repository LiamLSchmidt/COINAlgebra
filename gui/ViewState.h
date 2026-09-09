#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <vector>
class DecayQuiver;

namespace Studio {
// Membership belongs to the document. Layout/style/camera belong to a view.
// IDs are stable strings; graph members retain the document's level indices
// and unique transition names. Deletion remaps every level reference.
QJsonObject normalizeViewMetadata(QJsonObject metadata);
QJsonArray layoutGroups(const QJsonObject& metadata);
struct GraphSelection {
    std::vector<bool> levels;
    std::vector<bool> transitions;
    std::vector<bool> contextLevels;
    std::vector<bool> contextTransitions;
};
GraphSelection selectGraph(const DecayQuiver&, const QJsonObject& metadata, bool focus);
QJsonObject objectStyle(const QJsonObject& metadata, const QString& groupId,
                        const QString& objectId, bool transition);
void validateViewMetadata(const DecayQuiver&, const QJsonObject&);
void removeViewLevel(QJsonObject& metadata, int index);
void pruneViewTransitions(QJsonObject& metadata, const DecayQuiver&);
// Normalized height coordinates, ascending with energy. Uniform mode sorts by
// energy where available. Explicit energy modes reject missing energies.
std::vector<double> energyCoordinates(const DecayQuiver&, const QJsonObject&,
                                      std::vector<bool>& visible);
}
