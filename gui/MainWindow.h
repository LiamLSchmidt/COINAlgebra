#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>
#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Core/DecayVector.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QuiverView;

class DecayQuiver;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private slots:
    void addLevel();
    void addTransition();
    void onLevelRenamed(int index, const QString& newName);
    void onTransitionEdited(int index, double probability);
    void importQuiver();
    void exportQuiver();
    void editLevelItem(QListWidgetItem* item);
    void editTransitionItem(QListWidgetItem* item);
    void showPathVectorBuilder();

private:
    void updateUI();
    
    QuiverView* fView;

    QLineEdit* fLevelName;
    QPushButton* fAddLevel;
    QPushButton* fExportButton;

    QComboBox* fSourceBox;
    QComboBox* fTargetBox;
    QLineEdit* fProbability;
    QPushButton* fAddTransition;

    QListWidget* fLevelsList;
    QListWidget* fTransitionsList;

    std::unique_ptr<DecayQuiver> fQuiver;
    // stored vectors created via the builder
    std::vector<DecayVector> fVectors;
};
