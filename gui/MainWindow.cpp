#include "MainWindow.h"

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

#include "COINAlgebra/DecayQuiver.h"
#include "COINAlgebra/DecayLevel.h"
#include "COINAlgebra/DecayVector.h"

MainWindow::MainWindow()
    : QMainWindow(nullptr)
, fQuiver(std::make_unique<DecayQuiver>())
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLay = new QHBoxLayout(central);

    // Left: controls
    auto* left = new QWidget(central);
    auto* v = new QVBoxLayout(left);

    v->addWidget(new QLabel("Add level"));
    fLevelName = new QLineEdit();
    v->addWidget(fLevelName);
    fAddLevel = new QPushButton("Add Level");
    v->addWidget(fAddLevel);

    v->addWidget(new QLabel("Add transition"));
    fSourceBox = new QComboBox();
    fTargetBox = new QComboBox();
    fProbability = new QLineEdit();
    fProbability->setText("1.0");
    fAddTransition = new QPushButton("Add Transition");
    fExportButton = new QPushButton("Export Quiver");
    QPushButton* builderBtn = new QPushButton("Paths & Vectors");

    v->addWidget(new QLabel("Source"));
    v->addWidget(fSourceBox);
    v->addWidget(new QLabel("Target"));
    v->addWidget(fTargetBox);
    v->addWidget(new QLabel("Probability"));
    v->addWidget(fProbability);
    v->addWidget(fAddTransition);
    v->addWidget(fExportButton);
    v->addWidget(builderBtn);

    v->addWidget(new QLabel("Levels"));
    fLevelsList = new QListWidget();
    v->addWidget(fLevelsList);

    v->addWidget(new QLabel("Transitions"));
    fTransitionsList = new QListWidget();
    v->addWidget(fTransitionsList);

    mainLay->addWidget(left, 0);

    // Right: view
    fView = new QuiverView();
    mainLay->addWidget(fView, 1);

    connect(fAddLevel, &QPushButton::clicked, this, &MainWindow::addLevel);
    connect(fAddTransition, &QPushButton::clicked, this, &MainWindow::addTransition);
    connect(fView, &QuiverView::levelRenamed, this, &MainWindow::onLevelRenamed);
    connect(fView, &QuiverView::transitionEdited, this, &MainWindow::onTransitionEdited);
    connect(fExportButton, &QPushButton::clicked, this, &MainWindow::exportQuiver);
    connect(builderBtn, &QPushButton::clicked, this, &MainWindow::showPathVectorBuilder);
    connect(fLevelsList, &QListWidget::itemDoubleClicked, this, &MainWindow::editLevelItem);
    connect(fTransitionsList, &QListWidget::itemDoubleClicked, this, &MainWindow::editTransitionItem);

    setWindowTitle("COINAlgebra Quiver Builder");
    resize(900, 600);

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
}

void MainWindow::exportQuiver()
{
    // serialize to JSON and save into examples/ with timestamped filename
    const auto& levels = fQuiver->GetLevels();
    const auto& trans = fQuiver->GetTransitions();

    QJsonObject root;
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
        QDialog dlg(this);
        dlg.setWindowTitle("Edit Transition");
        QFormLayout form(&dlg);
        QComboBox* srcCombo = new QComboBox(&dlg);
        QComboBox* tgtCombo = new QComboBox(&dlg);
        for (const auto* L : levels) {
            srcCombo->addItem(QString::fromStdString(L->GetName()));
            tgtCombo->addItem(QString::fromStdString(L->GetName()));
        }
        int currentSrc = 0;
        int currentTgt = 0;
        for (int i=0;i<(int)levels.size();++i) {
            if (levels[i] == tr->GetSource()) currentSrc = i;
            if (levels[i] == tr->GetTarget()) currentTgt = i;
        }
        srcCombo->setCurrentIndex(currentSrc);
        tgtCombo->setCurrentIndex(currentTgt);
        QDoubleSpinBox* probSpin = new QDoubleSpinBox(&dlg);
        probSpin->setRange(0.0, 1.0);
        probSpin->setDecimals(6);
        probSpin->setSingleStep(0.01);
        probSpin->setValue(tr->GetProbability());
        form.addRow("Source:", srcCombo);
        form.addRow("Target:", tgtCombo);
        form.addRow("Probability:", probSpin);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);
        form.addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        if (dlg.exec() == QDialog::Accepted) {
            DecayLevel* src = fQuiver->GetLevel(srcCombo->currentText().toStdString());
            DecayLevel* tgt = fQuiver->GetLevel(tgtCombo->currentText().toStdString());
            double p = probSpin->value();
            if (src && tgt) {
                tr->SetSource(src);
                tr->SetTarget(tgt);
                tr->SetProbability(p);
                updateUI();
            }
        }
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
