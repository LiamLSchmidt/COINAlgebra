#include "MainWindow.h"
#include "QuiverJson.h"
#include "TransitionEditor.h"
#include "COINAlgebra/Algebra/PathAlgebra.h"
#include "COINAlgebra/Algebra/PathProjectors.h"
#include "COINAlgebra/Probability/DecayProbability.h"
#include <QPlainTextEdit>
#include <QFontDatabase>
#include <sstream>
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
    auto* hint = new QLabel("DECAY SCHEME    ·    Drag to arrange  /  Right-click to edit", central);
    hint->setObjectName("canvasHint");
    canvas->addWidget(hint);
    fView = new QuiverView();
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
    connect(fView, &QuiverView::levelRenamed, this, &MainWindow::onLevelRenamed);
    connect(fView, &QuiverView::transitionEdited, this, &MainWindow::onTransitionEdited);
    connect(fExportButton, &QPushButton::clicked, this, &MainWindow::exportQuiver);
    connect(builderBtn, &QPushButton::clicked, this, &MainWindow::showPathVectorBuilder);
    connect(fLevelsList, &QListWidget::itemDoubleClicked, this, &MainWindow::editLevelItem);
    connect(fTransitionsList, &QListWidget::itemDoubleClicked, this, &MainWindow::editTransitionItem);

    setWindowTitle("DecayQuiver Studio");
    setMinimumSize(820, 640);
    resize(1440, 860);

    fView->setQuiver(fQuiver.get());
    updateUI();
}

void MainWindow::addLevel()
{
    const QString name = fLevelName->text().trimmed();
    if (name.isEmpty()) return;

    DecayLevel* lvl = fQuiver->AddLevel(name.toStdString());
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
    fAnalysisResult->setText("Uses the full quiver, including levels hidden by zoom.");
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
        fView->setQuiver(fQuiver.get());
        fLevelName->clear();
        fProbability->setText("1.0");
        updateUI();
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
    if (!fMetadata.isEmpty()) root["metadata"] = fMetadata;
    QJsonArray lvlArr;
    for (const auto* L : levels) {
        lvlArr.append(QString::fromStdString(L->GetName()));
    }
    root["levels"] = lvlArr;

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
    for (const auto& v : fVectors) {
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
        vecArr.append(vobj);
    }
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
            fQuiver->RemoveLevel(lvl);
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

    // actions
    connect(addToPath, &QPushButton::clicked, this, [=]() {
        QString name = transBox->currentText();
        pathList->addItem(name);
    });

    connect(createPathBtn, &QPushButton::clicked, this, [=]() {
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
        DecayPath dp(seq);
        QString info = QString::fromStdString(dp.ToString()) + "  prob=" + QString::number(dp.GetProbability(), 'g', 6);
        pathInfo->setText(info);
    });

    connect(addTermBtn, &QPushButton::clicked, this, [=]() {
        // require a path shown
        QString info = pathInfo->text();
        if (info.isEmpty()) return;
        double coef = coefSpin->value();
        vectorTerms->addItem(info + "  coef=" + QString::number(coef, 'g', 6));
    });

    if (dlg.exec() == QDialog::Accepted) {
        // on accept, convert listed terms into a DecayVector and store
        DecayVector vec;
        for (int i=0;i<vectorTerms->count();++i) {
            QString text = vectorTerms->item(i)->text();
            // parse coefficient from the end after 'coef='
            int idx = text.lastIndexOf("coef=");
            double coef = 1.0;
            QString pathStr = text;
            if (idx >= 0) {
                QString cs = text.mid(idx + 5);
                coef = cs.toDouble();
                pathStr = text.left(idx).trimmed();
            }
            // reconstruct path by matching the display substring to a created path via ToString
            // For simplicity, try to find a sequence matching names in pathList (best-effort)
            // Here we will create an empty stationary path as fallback
            DecayPath dp;
            // naive parse: extract transition names between '[' and ']' if present
            int b = pathStr.indexOf('[');
            int e = pathStr.indexOf(']');
            if (b >= 0 && e > b) {
                QString inside = pathStr.mid(b+1, e-b-1);
                QStringList parts = inside.split(",");
                std::vector<DecayTransition> seq;
                for (const QString& part : parts) {
                    QString name = part.trimmed();
                    // try match
                    for (const auto* t : fQuiver->GetTransitions()) {
                        if (QString::fromStdString(t->GetName()) == name) {
                            seq.push_back(*t);
                            break;
                        }
                    }
                }
                dp = DecayPath(seq);
            }
            vec.AddTerm(dp, coef);
        }
        if (!vec.Empty()) fVectors.push_back(vec);
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
