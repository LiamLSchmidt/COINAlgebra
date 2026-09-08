#include "TransitionEditor.h"
#include "COINAlgebra/Core/DecayLevel.h"
#include "COINAlgebra/Core/DecayTransition.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>

bool Studio::editTransition(QWidget* parent, DecayTransition& transition,
                            const std::vector<DecayLevel*>& levels)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Edit Transition");
    dialog.setMinimumWidth(340);
    QFormLayout form(&dialog);
    QComboBox source(&dialog), target(&dialog);
    source.setObjectName("transitionSource");
    target.setObjectName("transitionTarget");
    for (auto* level : levels) {
        source.addItem(QString::fromStdString(level->GetName()));
        target.addItem(QString::fromStdString(level->GetName()));
    }
    source.setCurrentIndex(-1);
    target.setCurrentIndex(-1);
    for (int i = 0; i < static_cast<int>(levels.size()); ++i) {
        if (levels[i] == transition.GetSource()) source.setCurrentIndex(i);
        if (levels[i] == transition.GetTarget()) target.setCurrentIndex(i);
    }
    QDoubleSpinBox probability(&dialog);
    probability.setObjectName("transitionProbability");
    probability.setRange(0.0, 1.0);
    probability.setDecimals(12);
    probability.setSingleStep(0.01);
    probability.setValue(transition.GetProbability());
    const double initialDisplayProbability = probability.value();
    form.addRow("Source:", &source);
    form.addRow("Target:", &target);
    form.addRow("Probability:", &probability);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form.addRow(&buttons);
    QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted || source.currentIndex() < 0 || target.currentIndex() < 0)
        return false;
    transition.SetSource(levels[source.currentIndex()]);
    transition.SetTarget(levels[target.currentIndex()]);
    // Avoid rounding the existing probability when only an endpoint changed.
    if (probability.value() != initialDisplayProbability)
        transition.SetProbability(probability.value());
    return true;
}
