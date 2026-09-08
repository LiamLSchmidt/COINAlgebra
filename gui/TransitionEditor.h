#pragma once
#include <vector>
class QWidget;
class DecayLevel;
class DecayTransition;

namespace Studio {
// Returns true only when all three fields have been accepted and applied.
bool editTransition(QWidget* parent, DecayTransition& transition,
                    const std::vector<DecayLevel*>& levels);
}
