#pragma once
#include "QuiverJson.h"
#include <unordered_map>
namespace Studio {
// Document metadata uses level indices, never editable display labels.
DecayVector branchingVector(const DecayQuiver&, const QJsonObject&);
DecayVector transitionVector(const DecayQuiver&);
double levelEnergy(const DecayQuiver&, const QJsonObject&, int);
QJsonArray readEfficiencyCsv(const QByteArray&);
std::unordered_map<std::string,double> efficiencyMap(const DecayQuiver&, const QJsonObject&);
std::unordered_map<std::string,double> conversionMap(const DecayQuiver&, const QJsonObject&);
void validateStudio(const DecayQuiver&, const QJsonObject&);
}
