#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>
#include <QJsonObject>
#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Core/DecayVector.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QuiverView;
class QLabel;

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
    void createDecayVector();
    void calculateFeeding();
    void calculateCoincidence();
    void showDecayTable();
    void showFeedingTable();

private:
    void removeLevel(int index);
    void editPopulations();
    void editGroups();
    void editSubquivers();
    void editViewSettings();
    void manageViews();
    void importEfficiencies();
    void showDetectionTable();
    void showEmissionTable();
    void showEfficiencyTable();
    void updateUI();
    void showVectorTable(const DecayVector& vector, const QString& title);
    DecayVector currentDecay() const;
    
    QuiverView* fView;
    QLineEdit* fQuiverTitle;

    QLineEdit* fLevelName;
    QLineEdit* fLevelEnergy;
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
    QJsonObject fMetadata;
    QLineEdit* fFeedingTransition;
    QLabel* fAnalysisResult;
    QComboBox* fCoincidenceA;
    QComboBox* fCoincidenceB;
    QPushButton* fCalculateCoincidence;
    QLabel* fCoincidenceResult;
    void invalidateCoincidence();
};
