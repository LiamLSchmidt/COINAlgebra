#include "MainWindow.h"
#include "QuiverJson.h"
#include "StudioModel.h"
#include "ViewState.h"
#include <QSignalBlocker>
#include <QTableWidget>
#include <QHeaderView>
#include <QColorDialog>
#include <QCheckBox>
#include "TransitionEditor.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include "COINAlgebra/Probability/DecayProbability.h"
#include <QPlainTextEdit>
#include <QFontDatabase>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <functional>
#include <map>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QPixmap>
#include <QStatusBar>
#include <QScrollArea>
#include <QResource>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>

#include "QuiverView.h"

#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Core/DecayLevel.h"
#include "COINAlgebra/Core/DecayVector.h"

MainWindow::MainWindow()
    : QMainWindow(nullptr)
, fQuiver(std::make_unique<DecayQuiver>())
{
    Q_INIT_RESOURCE(studio);
    QFile theme(":/studio/theme.qss");
    if (theme.open(QIODevice::ReadOnly)) setStyleSheet(QString::fromUtf8(theme.readAll()));
    setWindowIcon(QIcon(":/studio/logo.jpeg"));

    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(18, 14, 18, 6);
    layout->setSpacing(14);

    auto* header = new QWidget(central);
    header->setObjectName("header");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 4, 22, 4);
    auto* logo = new QLabel(header);
    logo->setObjectName("studioLogo");
    logo->setAccessibleName("DecayQuiver Studio logo");
    QPixmap logoImage(":/studio/logo.jpeg");
    const auto ratio = devicePixelRatioF();
    logoImage = logoImage.scaled(QSize(210, 105) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    logoImage.setDevicePixelRatio(ratio);
    logo->setPixmap(logoImage);
    logo->setFixedSize(210, 105);
    logo->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(logo);
    headerLayout->addSpacing(24);
    auto* heading = new QVBoxLayout();
    heading->setSpacing(6);
    heading->addStretch();
    auto* title = new QLabel("Decay Quiver Studio", header);
    title->setObjectName("workspaceTitle");
    title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    heading->addWidget(title);
    auto* subtitle = new QLabel("Build, explore and connect nuclear decay schemes.", header);
    subtitle->setObjectName("subtitle");
    subtitle->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    heading->addWidget(subtitle);
    heading->addStretch();
    headerLayout->addLayout(heading);
    headerLayout->addStretch();
    auto* badge = new QLabel("COINAlgebra", header);
    badge->setObjectName("badge");
    headerLayout->addWidget(badge, 0, Qt::AlignVCenter);
    layout->addWidget(header);

    auto* body = new QHBoxLayout();
    body->setSpacing(16);
    layout->addLayout(body, 1);
    auto* left = new QWidget(central);
    left->setObjectName("sidebar");
    auto* v = new QVBoxLayout(left);
    v->setContentsMargins(16, 12, 16, 16);
    v->setSpacing(7);
    const auto section = [&](const QString& text) {
        auto* label = new QLabel(text, left);
        label->setObjectName("sectionTitle");
        v->addWidget(label);
    };
    section("LEVELS & TRANSITIONS");
    fLevelName = new QLineEdit();
    fLevelName->setObjectName("levelName");
    fLevelName->setPlaceholderText("Level name, e.g. d2");
    fLevelName->setAccessibleName("Level name");
    v->addWidget(fLevelName);
    fLevelEnergy=new QLineEdit();
    fLevelEnergy->setObjectName("levelEnergy");
    fLevelEnergy->setPlaceholderText("Energy (keV), optional");
    fLevelEnergy->setAccessibleName("Level energy in keV");
    v->addWidget(fLevelEnergy);
    fAddLevel = new QPushButton("Add Level");
    v->addWidget(fAddLevel);

    auto* endpoints = new QHBoxLayout();
    auto* sourceColumn = new QVBoxLayout();
    auto* targetColumn = new QVBoxLayout();
    fSourceBox = new QComboBox();
    fSourceBox->setAccessibleName("Source level");
    fTargetBox = new QComboBox();
    fTargetBox->setAccessibleName("Target level");
    sourceColumn->addWidget(new QLabel("Source"));
    sourceColumn->addWidget(fSourceBox);
    targetColumn->addWidget(new QLabel("Target"));
    targetColumn->addWidget(fTargetBox);
    endpoints->addLayout(sourceColumn);
    endpoints->addLayout(targetColumn);
    v->addLayout(endpoints);
    fProbability = new QLineEdit("1.0");
    fProbability->setObjectName("probability");
    fProbability->setAccessibleName("Transition probability");
    v->addWidget(new QLabel("Probability"));
    v->addWidget(fProbability);
    fAddTransition = new QPushButton("Add Transition");
    fAddTransition->setObjectName("primaryButton");
    v->addWidget(fAddTransition);

    section("WORKSPACE");
    auto* importButton = new QPushButton("Import Quiver");
    importButton->setObjectName("importButton");
    importButton->setToolTip("Open a quiver from a JSON file");
    v->addWidget(importButton);
    fExportButton = new QPushButton("Export Quiver");
    v->addWidget(fExportButton);
    QPushButton* builderBtn = new QPushButton("Paths && Vectors");


    section("LEVELS");
    fLevelsList = new QListWidget();
    fLevelsList->setMinimumHeight(72);
    v->addWidget(fLevelsList, 1);
    section("TRANSITIONS");
    fTransitionsList = new QListWidget();
    fTransitionsList->setMinimumHeight(72);
    v->addWidget(fTransitionsList, 1);

    auto* scroll = new QScrollArea(central);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFixedWidth(280);
    scroll->setWidget(left);
    body->addWidget(scroll);

    auto* canvas = new QVBoxLayout();
    auto* titleRow = new QHBoxLayout();
    auto* titleLabel = new QLabel("Quiver title", central);
    fQuiverTitle = new QLineEdit(central);
    fQuiverTitle->setObjectName("quiverTitle");
    fQuiverTitle->setAccessibleName("Quiver title");
    fQuiverTitle->setPlaceholderText("Enter a title, e.g. 56Fe levels");
    titleLabel->setBuddy(fQuiverTitle);
    titleRow->addWidget(titleLabel);
    titleRow->addWidget(fQuiverTitle, 1);
    canvas->addLayout(titleRow);
    auto* hint = new QLabel("DECAY SCHEME    ·    Drag to arrange  /  Right-click to edit", central);
    hint->setObjectName("canvasHint");
    canvas->addWidget(hint);
    fView = new QuiverView();
    auto* viewTools = new QHBoxLayout();
    auto* mode = new QComboBox(); mode->addItems({"Quiver View", "Band View", "Focus View"});
    viewTools->addWidget(mode);
    auto* groups = new QPushButton("Groups…"); viewTools->addWidget(groups);
    connect(groups,&QPushButton::clicked,this,&MainWindow::editGroups);
    connect(mode,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int i) { fView->setMode(i); });
    auto* fit = new QPushButton("Fit"); viewTools->addWidget(fit);
    connect(fit,&QPushButton::clicked,this,[this] { fView->resetTransform(); fView->fitInView(fView->sceneRect(),Qt::KeepAspectRatio); });
    auto* energyScale = new QDoubleSpinBox(); energyScale->setRange(0.25,10); energyScale->setValue(1); energyScale->setSingleStep(.25); energyScale->setPrefix("Y × ");
    viewTools->addWidget(energyScale);
    connect(energyScale,QOverload<double>::of(&QDoubleSpinBox::valueChanged),this,[this](double v){ fView->setEnergyScale(v); });
    canvas->addLayout(viewTools);
    auto* moreTools=new QHBoxLayout();
    auto* subsets=new QPushButton("Subquivers…");moreTools->addWidget(subsets);connect(subsets,&QPushButton::clicked,this,&MainWindow::editSubquivers);
    auto* settings=new QPushButton("Focus / energy / style…");moreTools->addWidget(settings);connect(settings,&QPushButton::clicked,this,&MainWindow::editViewSettings);
    auto* views=new QPushButton("Saved views…");moreTools->addWidget(views);connect(views,&QPushButton::clicked,this,&MainWindow::manageViews);
    canvas->addLayout(moreTools);
    connect(fView,&QuiverView::appearanceChanged,this,[mode,energyScale](const QJsonObject& m) {
        QSignalBlocker a(mode),b(energyScale);auto view=m["view"].toObject();mode->setCurrentIndex(view.value("mode").toInt());energyScale->setValue(view.value("energy_scale").toDouble(1));
    });
    canvas->addWidget(fView, 1);
    body->addLayout(canvas, 1);
    auto* right = new QWidget(central);
    right->setObjectName("sidebar");
    auto* analysis = new QVBoxLayout(right);
    analysis->setContentsMargins(16, 12, 16, 16);
    analysis->setSpacing(10);
    auto* analysisTitle = new QLabel("PATHS & PROBABILITIES");
    analysisTitle->setObjectName("sectionTitle");
    analysis->addWidget(analysisTitle);
    analysis->addWidget(builderBtn);
    auto* create = new QPushButton("Create Decay Vector");
    create->setObjectName("createDecayVector");
    analysis->addWidget(create);
    auto* decayTable = new QPushButton("Print Decay Vector Table");
    analysis->addWidget(decayTable);
    analysis->addWidget(new QLabel("Transition name"));
    fFeedingTransition = new QLineEdit();
    fFeedingTransition->setObjectName("feedingTransition");
    fFeedingTransition->setPlaceholderText("Select a transition on the left");
    analysis->addWidget(fFeedingTransition);
    auto* feeding = new QPushButton("Calculate Feeding Probability");
    analysis->addWidget(feeding);
    auto* feedingTable = new QPushButton("Print Feeding Table");
    analysis->addWidget(feedingTable);
    fAnalysisResult = new QLabel("Uses the full quiver, including levels hidden by zoom.");
    fAnalysisResult->setObjectName("analysisResult");
    fAnalysisResult->setWordWrap(true);
    fAnalysisResult->setTextInteractionFlags(Qt::TextSelectableByMouse);
    analysis->addWidget(fAnalysisResult);
    auto* populations = new QPushButton("Initial populations / energies…");
    analysis->addWidget(populations);
    connect(populations, &QPushButton::clicked, this, &MainWindow::editPopulations);
    for (bool branching : {true, false}) {
        auto* button = new QPushButton(branching ? "Show Branching Vector" : "Show Transition Vector");
        analysis->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, branching] {
            try { showVectorTable(branching ? Studio::branchingVector(*fQuiver,fMetadata) : Studio::transitionVector(*fQuiver), branching ? "Branching Vector" : "Transition Vector"); }
            catch (const std::exception& e) { QMessageBox::warning(this,"Vector",e.what()); }
        });
    }
    analysis->addWidget(new QLabel("DETECTION EFFICIENCY"));
    auto* efficiencies = new QPushButton("Import Efficiency CSV…");
    efficiencies->setToolTip("GRIFFIN Energy[keV],HPGe or energy_keV,efficiency; fractions 0–1; linear interpolation");
    analysis->addWidget(efficiencies);
    connect(efficiencies,&QPushButton::clicked,this,&MainWindow::importEfficiencies);
    auto* efficiencyTable=new QPushButton("Show Transition Efficiencies");
    analysis->addWidget(efficiencyTable);
    connect(efficiencyTable,&QPushButton::clicked,this,&MainWindow::showEfficiencyTable);
    auto* emission=new QPushButton("Show Gamma Emission Probabilities");
    analysis->addWidget(emission);
    connect(emission,&QPushButton::clicked,this,&MainWindow::showEmissionTable);
    auto* detection = new QPushButton("Show Detection Probabilities");
    analysis->addWidget(detection);
    connect(detection,&QPushButton::clicked,this,&MainWindow::showDetectionTable);
    auto* coincidenceTitle=new QLabel("TWO-TRANSITION COINCIDENCE");
    coincidenceTitle->setObjectName("sectionTitle");
    analysis->addWidget(coincidenceTitle);
    fCoincidenceA=new QComboBox();fCoincidenceA->setObjectName("coincidenceA");
    fCoincidenceB=new QComboBox();fCoincidenceB->setObjectName("coincidenceB");
    fCoincidenceA->setAccessibleName("Coincidence transition A");
    fCoincidenceB->setAccessibleName("Coincidence transition B");
    for(auto* box:{fCoincidenceA,fCoincidenceB}) {
        box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        box->setMinimumContentsLength(12);
    }
    analysis->addWidget(new QLabel("Transition A"));analysis->addWidget(fCoincidenceA);
    analysis->addWidget(new QLabel("Transition B"));analysis->addWidget(fCoincidenceB);
    fCalculateCoincidence=new QPushButton("Calculate Coincidence Probability");
    analysis->addWidget(fCalculateCoincidence);
    auto* coincidenceHint=new QLabel("Either order, using the full decay scheme. Detection assumes independent efficiencies.");
    coincidenceHint->setWordWrap(true);analysis->addWidget(coincidenceHint);
    fCoincidenceResult=new QLabel();fCoincidenceResult->setObjectName("coincidenceResult");
    fCoincidenceResult->setWordWrap(true);fCoincidenceResult->setTextInteractionFlags(Qt::TextSelectableByMouse);
    analysis->addWidget(fCoincidenceResult);
    connect(fCalculateCoincidence,&QPushButton::clicked,this,&MainWindow::calculateCoincidence);
    connect(fCoincidenceA,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this]{invalidateCoincidence();});
    connect(fCoincidenceB,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this]{invalidateCoincidence();});
    analysis->addStretch();
    auto* analysisScroll = new QScrollArea(central);
    analysisScroll->setWidgetResizable(true);
    analysisScroll->setFrameShape(QFrame::NoFrame);
    analysisScroll->setFixedWidth(280);
    analysisScroll->setWidget(right);
    body->addWidget(analysisScroll);
    connect(create, &QPushButton::clicked, this, &MainWindow::createDecayVector);
    connect(feeding, &QPushButton::clicked, this, &MainWindow::calculateFeeding);
    connect(decayTable, &QPushButton::clicked, this, &MainWindow::showDecayTable);
    connect(feedingTable, &QPushButton::clicked, this, &MainWindow::showFeedingTable);
    connect(fTransitionsList, &QListWidget::currentTextChanged, fFeedingTransition, &QLineEdit::setText);
    connect(importButton, &QPushButton::clicked, this, &MainWindow::importQuiver);

    connect(fAddLevel, &QPushButton::clicked, this, &MainWindow::addLevel);
    connect(fAddTransition, &QPushButton::clicked, this, &MainWindow::addTransition);
    connect(fView,&QuiverView::levelDeleteRequested,this,&MainWindow::removeLevel);
    connect(fView,&QuiverView::appearanceChanged,this,[this](const QJsonObject& m){ fMetadata=m; });
    connect(fView, &QuiverView::levelRenamed, this, &MainWindow::onLevelRenamed);
    connect(fView, &QuiverView::transitionEdited, this, &MainWindow::onTransitionEdited);
    connect(fExportButton, &QPushButton::clicked, this, &MainWindow::exportQuiver);
    connect(builderBtn, &QPushButton::clicked, this, &MainWindow::showPathVectorBuilder);
    connect(fLevelsList, &QListWidget::itemDoubleClicked, this, &MainWindow::editLevelItem);
    connect(fTransitionsList, &QListWidget::itemDoubleClicked, this, &MainWindow::editTransitionItem);

    setWindowTitle("DecayQuiver Studio");
    auto* author = new QLabel("Developed by Liam L. Schmidt", this);
    author->setObjectName("authorCredit");
    statusBar()->addPermanentWidget(author);
    setMinimumSize(820, 640);
    resize(1440, 860);

    fView->setQuiver(fQuiver.get());
    updateUI();
}

void MainWindow::addLevel()
{
    const QString name = fLevelName->text().trimmed();
    if (name.isEmpty()) return;

    try {
        const auto energy=fLevelEnergy->text().trimmed();
        if(energy.isEmpty()) fQuiver->AddLevel(name.toStdString());
        else {
            bool ok;const double value=energy.toDouble(&ok);
            if(!ok) throw std::invalid_argument("Enter a numeric level energy in keV, or leave it blank.");
            fQuiver->AddLevel(name.toStdString(),value);
        }
    } catch(const std::exception& e) {QMessageBox::warning(this,"Add Level",e.what());return;}
    if (fMetadata.contains("branching")) { auto a=fMetadata["branching"].toArray(); a.append(0.); fMetadata["branching"]=a; }
    updateUI();
}

void MainWindow::addTransition()
{
    const int s = fSourceBox->currentIndex();
    const int t = fTargetBox->currentIndex();
    if (s < 0 || t < 0) return;

    QString sname = fSourceBox->currentText();
    QString tname = fTargetBox->currentText();

    DecayLevel* src = fQuiver->GetLevel(sname.toStdString());
    DecayLevel* tgt = fQuiver->GetLevel(tname.toStdString());
    if (!src || !tgt) return;

    double p = fProbability->text().toDouble();
    std::string tlabel = sname.toStdString() + "->" + tname.toStdString();

    // prevent creating duplicate transitions: check by name or by identical source/target
    if (fQuiver->GetTransition(tlabel) != nullptr || fQuiver->HasDirectTransition(src, tgt)) {
        QMessageBox::warning(this, "Transition Exists", "A transition with the same name or same source/target already exists.");
        return;
    }

    try {
        DecayTransition* tr = fQuiver->AddTransition(tlabel, src, tgt, p);
        Q_UNUSED(tr);
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Add Transition Failed", QString::fromStdString(ex.what()));
        return;
    }

    updateUI();
}

void MainWindow::updateUI()
{
    fMetadata=Studio::normalizeViewMetadata(fMetadata);
    Studio::pruneViewTransitions(fMetadata,*fQuiver);
    if(fMetadata.contains("conversion_coefficients")) {
        auto coefficients=fMetadata["conversion_coefficients"].toObject();
        for(auto it=coefficients.begin();it!=coefficients.end();) {
            if(!fQuiver->GetTransition(it.key().toStdString())) it=coefficients.erase(it);else ++it;
        }
        fMetadata["conversion_coefficients"]=coefficients;
    }
    fAnalysisResult->setText("Uses the full quiver, including levels hidden by zoom.");
    // Rebind saved path copies after edits and discard paths whose objects were removed.
    for(auto& vector:fVectors) {
        DecayVector rebound;
        for(const auto& term:vector.GetTerms()) {
            try {
                if(term.path.IsStationary()) {
                    const auto& levels=fQuiver->GetLevels();
                    if(std::find(levels.begin(),levels.end(),term.path.GetSource())!=levels.end()) rebound.AddTerm(term.path,term.coefficient);
                } else {
                    std::vector<DecayTransition> path; bool valid=true;
                    for(const auto& old:term.path.GetTransitions()) { auto* t=fQuiver->GetTransition(old.GetName()); if(!t) { valid=false; break; } path.push_back(*t); }
                    if(valid) rebound.AddTerm(DecayPath(path),term.coefficient);
                }
            } catch(const std::exception&) { /* An edited endpoint can invalidate a saved path. */ }
        }
        vector=std::move(rebound);
    }
    // repopulate combos and lists from fQuiver
    fSourceBox->clear();
    fTargetBox->clear();
    fLevelsList->clear();
    fTransitionsList->clear();

    const auto& levels = fQuiver->GetLevels();
    for (const auto* lvl : levels) {
        QString name = QString::fromStdString(lvl->GetName());
        fSourceBox->addItem(name);
        fTargetBox->addItem(name);
        fLevelsList->addItem(name);
    }
    const auto& trans = fQuiver->GetTransitions();
    for (const auto* tr : trans) {
        fTransitionsList->addItem(QString::fromStdString(tr->GetName()));
    }

    const QString previousA=fCoincidenceA->currentText(),previousB=fCoincidenceB->currentText();
    {
        QSignalBlocker a(fCoincidenceA),b(fCoincidenceB);
        fCoincidenceA->clear();fCoincidenceB->clear();
        for(auto* t:trans) {
            const auto name=QString::fromStdString(t->GetName());
            fCoincidenceA->addItem(name);fCoincidenceB->addItem(name);
        }
        const int aIndex=fCoincidenceA->findText(previousA),bIndex=fCoincidenceB->findText(previousB);
        fCoincidenceA->setCurrentIndex(aIndex>=0?aIndex:(trans.empty()?-1:0));
        fCoincidenceB->setCurrentIndex(bIndex>=0?bIndex:(trans.size()>1?1:trans.empty()?-1:0));
    }
    invalidateCoincidence();
    fView->setMetadata(fMetadata);
    fView->refresh();
    statusBar()->showMessage(QString("%1 levels   ·   %2 transitions   ·   %3 vectors")
        .arg(levels.size()).arg(trans.size()).arg(fVectors.size()));
}

void MainWindow::importQuiver()
{
    const QString filename = QFileDialog::getOpenFileName(
        this, "Import Quiver", QDir::currentPath(), "Quiver JSON (*.json);;All files (*)");
    if (filename.isEmpty()) return;
    try {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error(file.errorString().toStdString());
        auto document = Studio::readJson(file.readAll());
        if (!fQuiver->GetLevels().empty() || !fVectors.empty()) {
            const auto answer = QMessageBox::question(this, "Replace current quiver?",
                "Importing will replace the current quiver and its vectors. "
                "Export first if you want to keep them.", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes) return;
        }
        // Switch only after parsing and validation have completed successfully.
        fVectors.clear();
        fView->setQuiver(nullptr);
        fQuiver = std::move(document.quiver);
        fVectors = std::move(document.vectors);
        fMetadata = document.metadata;
        fQuiverTitle->setText(document.title);
        fView->setQuiver(fQuiver.get());
        fLevelName->clear();
        fLevelEnergy->clear();
        fProbability->setText("1.0");
        updateUI();
        if(fMetadata.contains("view")) fView->restoreView(fMetadata["view"].toObject());
        statusBar()->showMessage("Imported " + QFileInfo(filename).fileName(), 6000);
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "Import Failed", QString::fromUtf8(error.what()));
    }
}

void MainWindow::exportQuiver()
{
    // serialize to JSON and save into examples/ with timestamped filename
    const auto& levels = fQuiver->GetLevels();
    const auto& trans = fQuiver->GetTransitions();

    QJsonObject root;
    if(fMetadata.contains("view")) fMetadata["view"]=fView->viewState();
    // Persist the authoritative initial populations even when they came from a default.
    try {
        auto branch=Studio::branchingVector(*fQuiver,fMetadata); QJsonArray fractions;
        for(auto* level:levels) { double p=0; for(const auto& term:branch.GetTerms()) if(term.path.GetSource()==level) p+=term.coefficient; fractions.append(p); }
        if(!levels.empty()) { auto candidate=fMetadata; candidate["branching"]=fractions; Studio::branchingVector(*fQuiver,candidate); fMetadata=candidate; }
    } catch(const std::exception& e) { QMessageBox::warning(this,"Export Failed",e.what()); return; }
    if (!fQuiverTitle->text().isEmpty()) root["title"] = fQuiverTitle->text();
    if (!fMetadata.isEmpty()) root["metadata"] = fMetadata;
    QJsonArray lvlArr;
    for (const auto* L : levels) {
        lvlArr.append(QString::fromStdString(L->GetName()));
    }
    root["levels"] = lvlArr;
    QJsonArray energyArray;
    for(const auto* level:levels)
        energyArray.append(level->HasEnergy()?QJsonValue(level->GetEnergy()):QJsonValue(QJsonValue::Null));
    root["level_energies_keV"]=energyArray;

    QJsonArray trArr;
    for (const auto* T : trans) {
        QJsonObject o;
        o["name"] = QString::fromStdString(T->GetName());
        // find indices
        int si=-1, ti=-1;
        for (int i=0;i<(int)levels.size();++i) {
            if (levels[i] == T->GetSource()) si = i;
            if (levels[i] == T->GetTarget()) ti = i;
        }
        o["source_index"] = si;
        o["target_index"] = ti;
        o["probability"] = T->GetProbability();
        trArr.append(o);
    }
    root["transitions"] = trArr;

    // include any built vectors
    QJsonArray vecArr;
    const auto serializeVector = [](const DecayVector& v) {
        QJsonObject vobj;
        QJsonArray termsArr;
        for (const auto& term : v.GetTerms()) {
            QJsonObject to;
            // path as sequence of transition names
            QJsonArray pathArr;
            for (const auto& ttr : term.path.GetTransitions()) {
                pathArr.append(QString::fromStdString(ttr.GetName()));
            }
            to["path_names"] = pathArr;
            if (term.path.IsStationary())
                to["stationary_level"] = QString::fromStdString(term.path.GetSource()->GetName());
            to["path_probability"] = term.path.GetProbability();
            to["coefficient"] = term.coefficient;
            termsArr.append(to);
        }
        vobj["terms"] = termsArr;
        vobj["display"] = QString::fromStdString(v.ToString());
        return vobj;
    };
    for(const auto& v:fVectors) vecArr.append(serializeVector(v));
    if(!levels.empty()) root["analysis_vectors"]=QJsonObject{
        {"branching",serializeVector(Studio::branchingVector(*fQuiver,fMetadata))},
        {"transition",serializeVector(Studio::transitionVector(*fQuiver))},
        {"decay",serializeVector(currentDecay())}};
    root["vectors"] = vecArr;

    QJsonDocument doc(root);
    QString baseName = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString filename = QString("quiver-") + baseName + ".json";

    // find examples/ dir relative to current working dir
    QStringList tryDirs;
    tryDirs << QDir::currentPath() + "/examples"
            << QDir::currentPath() + "/../examples"
            << QDir::currentPath() + "/../../examples"
            << QDir::currentPath() + "/../../../examples";
    QString outDir;
    for (const QString& d : tryDirs) {
        QDir dir(d);
        if (dir.exists()) { outDir = d; break; }
    }
    if (outDir.isEmpty()) {
        outDir = QDir::currentPath() + "/examples";
        QDir().mkpath(outDir);
    }

    QString outPath = QDir(outDir).filePath(filename);
    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Export Failed", "Could not write file: " + outPath);
        return;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();

    QMessageBox::information(this, "Exported", "Saved quiver to: " + outPath);
}

void MainWindow::editLevelItem(QListWidgetItem* item)
{
    if (!item) return;
    int idx = fLevelsList->row(item);
    const auto& levels = fQuiver->GetLevels();
    if (idx < 0 || idx >= (int)levels.size()) return;
    DecayLevel* lvl = levels[idx];

    QMessageBox msg(this);
    msg.setWindowTitle("Level");
    msg.setText("Choose action for level: " + QString::fromStdString(lvl->GetName()));
    QPushButton* renameBtn = msg.addButton("Rename", QMessageBox::ActionRole);
    QPushButton* delBtn = msg.addButton("Delete", QMessageBox::DestructiveRole);
    msg.addButton("Cancel", QMessageBox::RejectRole);
    msg.exec();
    if (msg.clickedButton() == renameBtn) {
        bool ok=false;
        QString name = QInputDialog::getText(this, "Rename Level", "Name:", QLineEdit::Normal, QString::fromStdString(lvl->GetName()), &ok);
        if (ok && !name.isEmpty()) {
            lvl->SetName(name.toStdString());
            updateUI();
        }
    } else if (msg.clickedButton() == delBtn) {
        if (QMessageBox::question(this, "Confirm Delete", "Delete level and its transitions?") == QMessageBox::Yes) {
            removeLevel(idx);
            updateUI();
        }
    }
}

void MainWindow::editTransitionItem(QListWidgetItem* item)
{
    if (!item) return;
    int idx = fTransitionsList->row(item);
    const auto& trans = fQuiver->GetTransitions();
    const auto& levels = fQuiver->GetLevels();
    if (idx < 0 || idx >= (int)trans.size()) return;
    DecayTransition* tr = trans[idx];

    QMessageBox msg(this);
    msg.setWindowTitle("Transition");
    msg.setText("Choose action for transition: " + QString::fromStdString(tr->GetName()));
    QPushButton* editBtn = msg.addButton("Edit", QMessageBox::ActionRole);
    QPushButton* delBtn = msg.addButton("Delete", QMessageBox::DestructiveRole);
    msg.addButton("Cancel", QMessageBox::RejectRole);
    msg.exec();
    if (msg.clickedButton() == editBtn) {
        if (Studio::editTransition(this, *tr, levels)) updateUI();
    } else if (msg.clickedButton() == delBtn) {
        if (QMessageBox::question(this, "Confirm Delete", "Delete transition?") == QMessageBox::Yes) {
            fQuiver->RemoveTransition(tr);
            updateUI();
        }
    }
}

void MainWindow::onLevelRenamed(int index, const QString& newName)
{
    Q_UNUSED(index);
    Q_UNUSED(newName);
    updateUI();
}

void MainWindow::onTransitionEdited(int index, double probability)
{
    Q_UNUSED(index);
    Q_UNUSED(probability);
    updateUI();
}

void MainWindow::showPathVectorBuilder()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Path & Vector Builder");
    QVBoxLayout* main = new QVBoxLayout(&dlg);

    // transition selector to build a path sequence
    QHBoxLayout* addRow = new QHBoxLayout();
    QComboBox* transBox = new QComboBox(&dlg);
    const auto& trans = fQuiver->GetTransitions();
    for (const auto* t : trans) transBox->addItem(QString::fromStdString(t->GetName()));
    QPushButton* addToPath = new QPushButton("Add to Path", &dlg);
    addRow->addWidget(transBox);
    addRow->addWidget(addToPath);
    main->addLayout(addRow);

    QListWidget* pathList = new QListWidget(&dlg);
    main->addWidget(new QLabel("Path sequence:"));
    main->addWidget(pathList);

    QPushButton* createPathBtn = new QPushButton("Create Path (show prob)", &dlg);
    main->addWidget(createPathBtn);
    QLabel* pathInfo = new QLabel("", &dlg);
    main->addWidget(pathInfo);

    // vector builder
    main->addWidget(new QLabel("Vector terms (path + coefficient):"));
    QListWidget* vectorTerms = new QListWidget(&dlg);
    main->addWidget(vectorTerms);
    QHBoxLayout* coefRow = new QHBoxLayout();
    QDoubleSpinBox* coefSpin = new QDoubleSpinBox(&dlg);
    coefSpin->setRange(-1e6, 1e6);
    coefSpin->setDecimals(6);
    coefSpin->setValue(1.0);
    QPushButton* addTermBtn = new QPushButton("Add Term to Vector", &dlg);
    coefRow->addWidget(coefSpin);
    coefRow->addWidget(addTermBtn);
    main->addLayout(coefRow);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    main->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    DecayVector pendingVector;
    DecayPath pendingPath;
    bool pathValid=false;
    // actions
    connect(addToPath, &QPushButton::clicked, this, [=, &pathValid]() {
        pathValid=false; pathInfo->clear();
        QString name = transBox->currentText();
        pathList->addItem(name);
    });

    connect(createPathBtn, &QPushButton::clicked, this, [=, &pendingPath, &pathValid]() {
        // build DecayPath from listed transitions and show probability
        std::vector<DecayTransition> seq;
        for (int i=0;i<pathList->count();++i) {
            QString tname = pathList->item(i)->text();
            // find transition by name
            for (const auto* t : fQuiver->GetTransitions()) {
                if (QString::fromStdString(t->GetName()) == tname) {
                    seq.push_back(*t);
                    break;
                }
            }
        }
        if(seq.empty()) return;
        try { pendingPath=DecayPath(seq); pathValid=true; }
        catch(const std::exception& e) { pathValid=false; QMessageBox::warning(this,"Path",e.what()); return; }
        const auto& dp=pendingPath;
        QString info = QString::fromStdString(dp.ToString()) + "  prob=" + QString::number(dp.GetProbability(), 'g', 6);
        pathInfo->setText(info);
    });

    connect(addTermBtn, &QPushButton::clicked, this, [=, &pendingVector, &pendingPath, &pathValid]() {
        // require a path shown
        QString info = pathInfo->text();
        if (info.isEmpty() || !pathValid) return;
        double coef = coefSpin->value();
        pendingVector.AddTerm(pendingPath,coef);
        vectorTerms->addItem(info + "  coef=" + QString::number(coef, 'g', 6));
    });

    if (dlg.exec() == QDialog::Accepted) {
        if (!pendingVector.Empty()) fVectors.push_back(pendingVector);
        updateUI();
    }
}

DecayVector MainWindow::currentDecay() const
{
    return Studio::createDecayVector(*fQuiver, fMetadata);
}

namespace {
// Feeding probabilities require finite, acyclic decay schemes.
void checkFeedingSize(const DecayQuiver& quiver) {
    std::map<const DecayLevel*, int> state;
    std::function<void(const DecayLevel*)> visit = [&](const DecayLevel* l) {
        if (state[l] == 1) throw std::runtime_error("Feeding requires an acyclic quiver.");
        if (state[l] == 2) return;
        state[l] = 1;
        for (auto* t : quiver.GetTransitions())
            if (t->GetSource() == l) visit(t->GetTarget());
        state[l] = 2;
    };
    for (auto* l : quiver.GetLevels()) visit(l);
}
}

void MainWindow::createDecayVector()
{
    try {
        auto decay = currentDecay();
        const auto count = decay.Size();
        fVectors.push_back(std::move(decay));
        updateUI();
        fAnalysisResult->setText(QString("Created decay vector %1 with %2 terms. Included in Export Quiver.").arg(fVectors.size()).arg(count));
    } catch (const std::exception& e) { QMessageBox::warning(this, "Decay Vector", e.what()); }
}

void MainWindow::calculateFeeding()
{
    try {
        const auto* t = fQuiver->GetTransition(fFeedingTransition->text().toStdString());
        if (!t) throw std::runtime_error("Select or enter an existing transition name.");
        const auto decay = currentDecay();
        checkFeedingSize(*fQuiver);
        PathAlgebra algebra(*fQuiver);
        PathProjectors projectors;
        DecayProbability probability(algebra, projectors);
        const double value = probability.FeedingProbability(decay, DecayPath(std::vector<DecayTransition>{*t}));
        fAnalysisResult->setText(QString("Feeding probability: %1").arg(value, 0, 'g', 12));
    } catch (const std::exception& e) { QMessageBox::warning(this, "Feeding Probability", e.what()); }
}

void MainWindow::showVectorTable(const DecayVector& vector, const QString& title)
{
    std::ostringstream stream;
    vector.PrintTable(stream);
    vector.PrintTable();
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.resize(1000, 600);
    auto* layout = new QVBoxLayout(&dialog);
    auto* text = new QPlainTextEdit(QString::fromStdString(stream.str()));
    text->setReadOnly(true);
    text->setLineWrapMode(QPlainTextEdit::NoWrap);
    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(text);
    auto* save = new QPushButton("Save Table…");
    layout->addWidget(save);
    connect(save, &QPushButton::clicked, &dialog, [&] {
        const auto path = QFileDialog::getSaveFileName(&dialog, "Save Table", "decay-table.txt", "Text (*.txt)");
        if (path.isEmpty()) return;
        QFile file(path);
        const auto bytes = text->toPlainText().toUtf8();
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size())
            QMessageBox::warning(&dialog, "Save Table", "Could not save table.");
    });
    dialog.exec();
}

void MainWindow::showDecayTable()
{
    try { showVectorTable(currentDecay(), "Decay Vector — coefficients (PrintTable)"); }
    catch (const std::exception& e) { QMessageBox::warning(this, "Decay Table", e.what()); }
}

void MainWindow::showFeedingTable()
{
    try {
        const auto decay = currentDecay();
        checkFeedingSize(*fQuiver);
        PathAlgebra algebra(*fQuiver);
        PathProjectors projectors;
        DecayProbability probability(algebra, projectors);
        showVectorTable(probability.FeedingVector(decay), "Feeding Vector — probabilities (PrintTable)");
    } catch (const std::exception& e) { QMessageBox::warning(this, "Feeding Table", e.what()); }
}

void MainWindow::editPopulations() {
    QDialog dialog(this); dialog.setWindowTitle("Initial level populations and energies"); dialog.resize(700,500);
    auto* layout=new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel("Initial fractions must sum to 1. Energies are in keV."));
    auto* table=new QTableWidget(fQuiver->GetLevels().size(),3); table->setHorizontalHeaderLabels({"Level","Initial fraction","Energy (keV)"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); layout->addWidget(table);
    DecayVector branch;
    try { branch=Studio::branchingVector(*fQuiver,fMetadata); } catch(const std::exception&) {}
    for(int i=0;i<table->rowCount();++i) {
        auto* level=fQuiver->GetLevels()[i]; auto* label=new QTableWidgetItem(QString::fromStdString(level->GetName())); label->setFlags(label->flags() & ~Qt::ItemIsEditable); table->setItem(i,0,label);
        double p=0; for(const auto& term:branch.GetTerms()) if(term.path.GetSource()==level) p+=term.coefficient;
        table->setItem(i,1,new QTableWidgetItem(QString::number(p,'g',16)));
        QString energy; try { energy=QString::number(Studio::levelEnergy(*fQuiver,fMetadata,i),'g',16); } catch(const std::exception&) {}
        table->setItem(i,2,new QTableWidgetItem(energy));
    }
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel); layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&] {
        try {
            auto metadata=fMetadata; QJsonArray fractions; QJsonObject energies;
            for(int i=0;i<table->rowCount();++i) {
                bool ok; double p=table->item(i,1)->text().toDouble(&ok); if(!ok) throw std::runtime_error("Invalid initial fraction."); fractions.append(p);
                auto text=table->item(i,2)->text().trimmed(); if(!text.isEmpty()) { double e=text.toDouble(&ok); if(!ok || !std::isfinite(e) || e<0) throw std::runtime_error("Invalid energy."); energies[QString::number(i)]=e; }
            }
            metadata["branching"]=fractions; metadata.remove("level_energies_keV");
            // Validate against the candidate core energies, restoring all old
            // values if branching/calibration validation fails.
            QJsonArray old;
            for(auto* level:fQuiver->GetLevels()) old.append(level->HasEnergy()?QJsonValue(level->GetEnergy()):QJsonValue(QJsonValue::Null));
            try {
                for(int i=0;i<table->rowCount();++i) {
                    auto* level=fQuiver->GetLevels()[i];
                    if(energies.contains(QString::number(i))) level->SetEnergy(energies[QString::number(i)].toDouble());
                    else level->ClearEnergy();
                }
                Studio::validateStudio(*fQuiver,metadata);
            } catch(...) {
                for(int i=0;i<old.size();++i) {
                    if(old[i].isNull()) fQuiver->GetLevels()[i]->ClearEnergy();
                    else fQuiver->GetLevels()[i]->SetEnergy(old[i].toDouble());
                }
                throw;
            }
            fMetadata=metadata; dialog.accept(); updateUI();
        } catch(const std::exception& e) { QMessageBox::warning(&dialog,"Populations",e.what()); }
    }); dialog.exec();
}

void MainWindow::importEfficiencies() {
    auto path=QFileDialog::getOpenFileName(this,"Import efficiencies",QDir::currentPath()+"/data","CSV (*.csv)"); if(path.isEmpty()) return;
    try {
        QFile file(path); if(!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot read CSV.");
        auto metadata=fMetadata; metadata["efficiency_samples"]=Studio::readEfficiencyCsv(file.readAll()); const auto mapped=Studio::efficiencyMap(*fQuiver,metadata);
        metadata["efficiency_source"]=QFileInfo(path).fileName(); fMetadata=metadata;
        fView->setMetadata(fMetadata);
        invalidateCoincidence();
        fAnalysisResult->setText(QString("Loaded %1: efficiencies assigned to %2 transitions using |Esource − Etarget|. Applied after physical feeding.")
            .arg(QFileInfo(path).fileName()).arg(mapped.size()));
    } catch(const std::exception& e) { QMessageBox::warning(this,"Efficiency import",e.what()); }
}
void MainWindow::showDetectionTable() {
    try {
        checkFeedingSize(*fQuiver); PathAlgebra algebra(*fQuiver); PathProjectors projectors; DecayProbability probability(algebra,projectors);
        showVectorTable(probability.DetectionFeedingVector(currentDecay(),Studio::efficiencyMap(*fQuiver,fMetadata),Studio::conversionMap(*fQuiver,fMetadata)),fMetadata.contains("conversion_coefficients")?"Gamma detection — efficiency × IC-corrected emission":"Gamma detection — no IC coefficients (α=0)");
    } catch(const std::exception& e) { QMessageBox::warning(this,"Detection",e.what()); }
}

void MainWindow::removeLevel(int index) {
    auto* level=fQuiver->GetLevels().at(index);
    if(fMetadata.contains("branching")) {
        auto a=fMetadata["branching"].toArray();
        if(a[index].toDouble()>0) { QMessageBox::warning(this,"Delete level","Set this level’s initial population to zero and redistribute it before deleting."); return; }
        a.removeAt(index); fMetadata["branching"]=a;
    }
    for(const QString key:{"level_energies_keV","level_colors"}) {
        auto old=fMetadata[key].toObject(); QJsonObject next;
        for(auto it=old.begin();it!=old.end();++it) { int i=it.key().toInt(); if(i!=index) next[QString::number(i>index?i-1:i)]=it.value(); }
        if(fMetadata.contains(key)) fMetadata[key]=next;
    }
    Studio::removeViewLevel(fMetadata,index);
    fQuiver->RemoveLevel(level); updateUI();
}

void MainWindow::showEfficiencyTable() {
    try {
        const auto efficiencies=Studio::efficiencyMap(*fQuiver,fMetadata);
        const auto conversion=Studio::conversionMap(*fQuiver,fMetadata);
        const auto& levels=fQuiver->GetLevels();
        QDialog dialog(this);dialog.setWindowTitle("Transition efficiencies — "+fMetadata.value("efficiency_source").toString());dialog.resize(850,500);
        auto* layout=new QVBoxLayout(&dialog);
        auto* table=new QTableWidget(fQuiver->GetTransitions().size(),4);
        table->setHorizontalHeaderLabels({"Transition","Energy difference (keV)","Efficiency (fraction)","IC coefficient α"});
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        int row=0;
        for(auto* t:fQuiver->GetTransitions()) {
            auto a=std::find(levels.begin(),levels.end(),t->GetSource())-levels.begin();
            auto b=std::find(levels.begin(),levels.end(),t->GetTarget())-levels.begin();
            const double energy=std::abs(Studio::levelEnergy(*fQuiver,fMetadata,a)-Studio::levelEnergy(*fQuiver,fMetadata,b));
            table->setItem(row,0,new QTableWidgetItem(QString::fromStdString(t->GetName())));
            table->setItem(row,1,new QTableWidgetItem(QString::number(energy,'g',12)));
            table->setItem(row,2,new QTableWidgetItem(QString::number(efficiencies.at(t->GetName()),'g',12)));
            table->setItem(row,3,new QTableWidgetItem(QString::number(conversion.at(t->GetName()),'g',12)));++row;
        }
        layout->addWidget(table);dialog.exec();
    } catch(const std::exception& e) {QMessageBox::warning(this,"Transition efficiencies",e.what());}
}

void MainWindow::showEmissionTable() {
    try {
        checkFeedingSize(*fQuiver);PathAlgebra algebra(*fQuiver);PathProjectors projectors;DecayProbability probability(algebra,projectors);
        showVectorTable(probability.EmissionFeedingVector(currentDecay(),Studio::conversionMap(*fQuiver,fMetadata)),fMetadata.contains("conversion_coefficients")?"Gamma emission — IC-corrected feeding":"Gamma emission — no IC coefficients (α=0)");
    } catch(const std::exception& e) {QMessageBox::warning(this,"Gamma emission",e.what());}
}

void MainWindow::invalidateCoincidence()
{
    const bool valid=fCoincidenceA->currentIndex()>=0 && fCoincidenceB->currentIndex()>=0 &&
                     fCoincidenceA->currentText()!=fCoincidenceB->currentText();
    fCalculateCoincidence->setEnabled(valid);
    fCoincidenceResult->setText(valid?"Ready to calculate.":"Choose two different transitions.");
}

void MainWindow::calculateCoincidence()
{
    try {
        const auto* a=fQuiver->GetTransition(fCoincidenceA->currentText().toStdString());
        const auto* b=fQuiver->GetTransition(fCoincidenceB->currentText().toStdString());
        if(!a || !b || a==b) throw std::invalid_argument("Choose two different transitions.");
        checkFeedingSize(*fQuiver);
        PathAlgebra algebra(*fQuiver);PathProjectors projectors;
        DecayProbability probability(algebra,projectors);
        const auto decay=currentDecay();
        const DecayPath pathA(*a),pathB(*b);
        // For distinct edges of a DAG, the two orderings are disjoint events.
        // PathForm groups endpoint-equivalent arrows. Resolve each selected
        // arrow's share so parallel transitions remain individually selectable.
        const auto share=[&](const DecayTransition* selected) {
            double total=0;
            for(auto* t:fQuiver->GetTransitions())
                if(t->GetSource()==selected->GetSource() && t->GetTarget()==selected->GetTarget()) total+=t->GetProbability();
            return total>0?selected->GetProbability()/total:0.;
        };
        const double physical=(probability.CoincidenceProbability(decay,pathA,pathB)+
                               probability.CoincidenceProbability(decay,pathB,pathA))*share(a)*share(b);
        const auto alpha=Studio::conversionMap(*fQuiver,fMetadata);
        const double emission=physical/(1+alpha.at(a->GetName()))/(1+alpha.at(b->GetName()));
        QString text=QString("Physical: %1\nGamma emission: %2").arg(physical,0,'g',12).arg(emission,0,'g',12);
        if(!fMetadata.contains("conversion_coefficients")) text+="\nNo IC data: α=0 assumed.";
        if(fMetadata.contains("efficiency_samples")) {
            const auto efficiencies=Studio::efficiencyMap(*fQuiver,fMetadata);
            const double detection=emission*efficiencies.at(a->GetName())*efficiencies.at(b->GetName());
            text+=QString("\nDetected: %1").arg(detection,0,'g',12);
        } else text+="\nImport efficiencies for the detected probability.";
        fCoincidenceResult->setText(text);
    } catch(const std::exception& e) {
        fCoincidenceResult->setText("Calculation unavailable: "+QString::fromUtf8(e.what()));
    }
}
