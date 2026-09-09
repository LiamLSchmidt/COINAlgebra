#include "MainWindow.h"
#include "QuiverView.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QPixmap>
#include <QGraphicsScene>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <cassert>
#include <iostream>
#include <cmath>

QPushButton* button(QWidget* parent, const QString& text) {
    for (auto* b : parent->findChildren<QPushButton*>())
        if (QString(b->text()).replace("&&", "&") == text) return b;
    assert(false && "Missing button");
    return nullptr;
}

int main(int argc, char** argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    QTemporaryDir output;
    assert(output.isValid());
    QDir().mkpath(output.path() + "/examples");
    QDir::setCurrent(output.path());
    MainWindow window;
    window.show();
    app.processEvents();
    assert(window.windowTitle() == "DecayQuiver Studio");
    auto* logo = window.findChild<QLabel*>("studioLogo");
    assert(logo && !logo->pixmap(Qt::ReturnByValue).isNull());
    assert(button(&window, "Import Quiver")->mapTo(&window, QPoint()).y() <
           button(&window, "Export Quiver")->mapTo(&window, QPoint()).y());
    QList<QLineEdit*> edits{window.findChild<QLineEdit*>("levelName"),
                           window.findChild<QLineEdit*>("probability")};
    auto* quiverTitle = window.findChild<QLineEdit*>("quiverTitle");
    assert(quiverTitle && quiverTitle->text().isEmpty());
    quiverTitle->setText("56Fe levels");
    for (const auto& name : {"d0", "d1", "d2"}) {
        edits[0]->setText(name);
        window.findChild<QLineEdit*>("levelEnergy")->setText(QString::number(QString(name).right(1).toInt()*100));
        button(&window, "Add Level")->click();
    }
    auto combos = window.findChildren<QComboBox*>();
    assert(combos.size() == 5);
    assert(combos[0]->count() == 3);
    combos[0]->setCurrentText("d2");
    combos[1]->setCurrentText("d1");
    edits[1]->setText("0.4");
    button(&window, "Add Transition")->click();
    auto lists = window.findChildren<QListWidget*>();
    assert(lists.size() == 2 && lists[0]->count() == 3 && lists[1]->count() == 1);
    auto* view = window.findChild<QuiverView*>();
    assert(view && !view->scene()->items().empty());
    QTimer::singleShot(0, [&] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        assert(dialog && dialog->windowTitle() == "Path & Vector Builder");
        button(dialog, "Add to Path")->click();
        button(dialog, "Create Path (show prob)")->click();
        bool probabilityShown = false;
        for (auto* label : dialog->findChildren<QLabel*>())
            probabilityShown |= label->text().contains("prob=0.4");
        assert(probabilityShown);
        button(dialog, "Add Term to Vector")->click();
        dialog->accept();
    });
    button(&window, "Paths & Vectors")->click();
    QTimer::singleShot(0, [&] {
        auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        assert(dialog && dialog->windowTitle() == "Exported");
        dialog->accept();
    });
    button(&window, "Export Quiver")->click();
    auto files = QDir(output.path() + "/examples").entryList({"quiver-*.json"}, QDir::Files);
    assert(files.size() == 1);
    QFile file(output.path() + "/examples/" + files[0]);
    assert(file.open(QIODevice::ReadOnly));
    auto json = QJsonDocument::fromJson(file.readAll()).object();
    assert(json["title"].toString() == "56Fe levels");
    assert(json["levels"].toArray().size() == 3);
    assert(json["level_energies_keV"].toArray()==QJsonArray({0,100,200}));
    assert(!json["metadata"].toObject().contains("level_energies_keV"));
    auto transitions = json["transitions"].toArray();
    assert(transitions.size() == 1);
    assert(transitions[0].toObject()["probability"].toDouble() == 0.4);
    auto vectors = json["vectors"].toArray();
    assert(vectors.size() == 1);
    auto term = vectors[0].toObject()["terms"].toArray()[0].toObject();
    assert(term["path_names"].toArray().size() == 1);
    assert(term["path_probability"].toDouble() == 0.4);
    // Exercise the real import button/file dialog, not just the JSON parser.
    const auto import = [&](const QString& filename, const QString& expectedDialog,
                            QMessageBox::StandardButton answer) {
        QTimer driver;
        bool selected = false;
        bool responded = false;
        QObject::connect(&driver, &QTimer::timeout, [&] {
            if (auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget())) {
                if (!selected) {
                    selected = true;
                    dialog->setDirectory(QFileInfo(filename).absolutePath());
                    auto* filenameEdit = dialog->findChild<QLineEdit*>("fileNameEdit");
                    assert(filenameEdit);
                    filenameEdit->setText(QFileInfo(filename).fileName());
                    QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
                }
            } else if (auto* message = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
                assert(message->windowTitle() == expectedDialog);
                responded = true;
                message->button(answer)->click();
            }
        });
        driver.start(10);
        button(&window, "Import Quiver")->click();
        driver.stop();
        assert(selected && responded);
    };
    edits[0]->setText("temporary");
    button(&window, "Add Level")->click();
    assert(lists[0]->count() == 4);
    const QString exported = output.path() + "/examples/" + files[0];
    import(exported, "Replace current quiver?", QMessageBox::No);
    assert(lists[0]->count() == 4);
    import(exported, "Replace current quiver?", QMessageBox::Yes);
    assert(lists[0]->count() == 3 && lists[1]->count() == 1);
    assert(combos[0]->count() == 3);

    // An invalid document must not partially replace the open workspace.
    assert(quiverTitle->text() == "56Fe levels");
    QFile invalid(output.path() + "/invalid.json");
    assert(invalid.open(QIODevice::WriteOnly));
    invalid.write(R"({"levels":["bad"],"transitions":[{"name":"broken","source_index":0,"target_index":99,"probability":1}]})");
    invalid.close();
    import(invalid.fileName(), "Import Failed", QMessageBox::Ok);
    assert(quiverTitle->text() == "56Fe levels");
    assert(lists[0]->count() == 3 && lists[0]->item(0)->text() == "d0");
    assert(lists[1]->count() == 1);

    QTimer::singleShot(0, [&] {
        auto* message = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        assert(message && message->windowTitle() == "Exported");
        message->accept();
    });
    button(&window, "Export Quiver")->click();
    // Every exported file must preserve the imported levels, transitions and vector.
    for (const auto& filename : QDir(output.path() + "/examples").entryList({"quiver-*.json"}, QDir::Files)) {
        QFile roundTrip(output.path() + "/examples/" + filename);
        assert(roundTrip.open(QIODevice::ReadOnly));
        assert(QJsonDocument::fromJson(roundTrip.readAll()).object() == json);
    }
    assert(button(&window, "Paths & Vectors")->mapTo(&window, QPoint()).x() >
           button(&window, "Import Quiver")->mapTo(&window, QPoint()).x());
    button(&window, "Create Decay Vector")->click();
    assert(window.findChild<QLabel*>("analysisResult")->text().contains("Created decay vector"));
    window.findChild<QLineEdit*>("feedingTransition")->setText(lists[1]->item(0)->text());
    button(&window, "Calculate Feeding Probability")->click();
    assert(window.findChild<QLabel*>("analysisResult")->text().contains("Feeding probability:"));
    QTimer::singleShot(0, [&] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        assert(dialog && dialog->windowTitle().startsWith("Decay Vector"));
        dialog->accept();
    });
    button(&window, "Print Decay Vector Table")->click();
    // Exercise the document editors and saved-view controls through their real widgets.
    QTimer::singleShot(0,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog);
        auto* name=dialog->findChild<QLineEdit*>("groupName");assert(name);name->setText("Band A");
        auto* members=dialog->findChild<QListWidget*>("groupLevels");members->item(1)->setSelected(true);members->item(2)->setSelected(true);
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
    });button(&window,"Groups…")->click();
    QTimer::singleShot(0,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog);
        dialog->findChild<QLineEdit*>("subquiverName")->setText("Selected cascade");
        dialog->findChild<QListWidget*>("subquiverTransitions")->item(0)->setSelected(true);
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
    });button(&window,"Subquivers…")->click();assert(view->mode()==2);
    QTimer::singleShot(0,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog);
        dialog->findChild<QLineEdit*>("viewName")->setText("Focused cascade");
        button(dialog,"Save current view")->click();assert(dialog->findChild<QListWidget*>("savedViews")->count()==1);dialog->reject();
    });button(&window,"Saved views…")->click();
    view->setMode(1);
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());dialog->findChild<QListWidget*>("savedViews")->setCurrentRow(0);button(dialog,"Restore selected view")->click();dialog->reject();});
    button(&window,"Saved views…")->click();assert(view->mode()==2);
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());auto* table=dialog->findChild<QTableWidget*>();assert(table);for(int i=0;i<3;++i)table->item(i,2)->setText(QString::number(i*100));dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();});
    button(&window,"Initial populations / energies…")->click();
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());dialog->findChild<QComboBox*>("energyMode")->setCurrentText("compressed");dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();dialog->reject();});
    button(&window,"Focus / energy / style…")->click();
    QTimer::singleShot(0,[&]{
        auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());assert(dialog);
        const auto source=QDir(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()).filePath("../../data/GRIFFIN_Eff.csv");
        dialog->selectFile(QFileInfo(source).absoluteFilePath());QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection);
    });button(&window,"Import Efficiency CSV…")->click();
    assert(window.findChild<QLabel*>("analysisResult")->text().contains("GRIFFIN_Eff.csv"));
    // Changing the view must not drop newly imported physical calibration data.
    view->setMode(0);
    QTimer::singleShot(0,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog);
        auto* table=dialog->findChild<QTableWidget*>();assert(table && table->rowCount()==1);
        assert(table->item(0,1)->text().toDouble()==100.);
        assert(std::abs(table->item(0,2)->text().toDouble()-.45137141695282584)<1e-11);dialog->accept();
    });button(&window,"Show Transition Efficiencies")->click();
    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());assert(dialog && dialog->windowTitle()=="Exported");dialog->accept();});
    button(&window,"Export Quiver")->click();
    bool foundView=false;
    for(const auto& filename:QDir(output.path()+"/examples").entryList({"quiver-*.json"},QDir::Files)) {QFile savedFile(output.path()+"/examples/"+filename);assert(savedFile.open(QIODevice::ReadOnly));auto exported=QJsonDocument::fromJson(savedFile.readAll()).object();auto metadata=exported["metadata"].toObject();if(!metadata["saved_views"].toArray().empty()){foundView=true;assert(metadata["efficiency_samples"].toArray().size()==1991);assert(metadata["groups"].toArray().size()==1);assert(metadata["subquivers"].toArray()[0].toObject()["transitions"].toArray().size()==1);assert(metadata["view"].toObject()["energy"].toObject()["mode"]=="compressed");}}
    assert(foundView);
    view->setMode(1);
    const auto baPath=QDir(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()).filePath("../../examples/133Ba_gamma_quiver.json");
    import(QFileInfo(baPath).absoluteFilePath(),"Replace current quiver?",QMessageBox::Yes);
    QTimer::singleShot(0,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());assert(dialog && dialog->windowTitle().contains("IC-corrected"));
        const auto table=dialog->findChild<QPlainTextEdit*>()->toPlainText();
        assert(table.contains("0.33284709") && table.contains("0.61847632"));dialog->accept();
    });button(&window,"Show Gamma Emission Probabilities")->click();
    auto* coincidenceA=window.findChild<QComboBox*>("coincidenceA");
    auto* coincidenceB=window.findChild<QComboBox*>("coincidenceB");
    auto* coincidenceResult=window.findChild<QLabel*>("coincidenceResult");
    assert(coincidenceA && coincidenceB && coincidenceResult);
    coincidenceA->setCurrentText("gamma_437_to_81_2");coincidenceB->setCurrentText("gamma_81_to_0_0");
    button(&window,"Calculate Coincidence Probability")->click();
    auto result=coincidenceResult->text();assert(result.contains("Physical: 0.634185621194"));
    const double expectedEmission=.634185621194/(1+.0254)/(1+1.703);
    assert(result.contains("Gamma emission: "));
    auto emissionLine=result.split('\n')[1];assert(std::abs(emissionLine.mid(QString("Gamma emission: ").size()).toDouble()-expectedEmission)<1e-11);
    coincidenceA->setCurrentText("gamma_81_to_0_0");coincidenceB->setCurrentText("gamma_437_to_81_2");
    assert(coincidenceResult->text()=="Ready to calculate.");button(&window,"Calculate Coincidence Probability")->click();assert(coincidenceResult->text()==result);
    coincidenceB->setCurrentText("gamma_81_to_0_0");assert(!button(&window,"Calculate Coincidence Probability")->isEnabled());
    coincidenceA->setCurrentText("gamma_437_to_161_1");coincidenceB->setCurrentText("gamma_437_to_81_2");
    button(&window,"Calculate Coincidence Probability")->click();assert(coincidenceResult->text().startsWith("Physical: 0\nGamma emission: 0"));

    QTimer::singleShot(0,[&]{auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());assert(dialog);dialog->selectFile(QDir(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()).absoluteFilePath("../../data/GRIFFIN_Eff.csv"));QMetaObject::invokeMethod(dialog,"accept",Qt::QueuedConnection);});
    button(&window,"Import Efficiency CSV…")->click();
    coincidenceA->setCurrentText("gamma_437_to_81_2");coincidenceB->setCurrentText("gamma_81_to_0_0");
    view->focusOnLevel(1); // Hidden transitions must still contribute to analysis.
    button(&window,"Calculate Coincidence Probability")->click();
    assert(coincidenceResult->text().contains("Detected: 0.0264318814936"));
    if (argc > 1) assert(window.grab().save(argv[1]));
    QFile parallel(output.path()+"/parallel.json");assert(parallel.open(QIODevice::WriteOnly));
    parallel.write(R"({"levels":["ground","middle","upper"],"transitions":[
        {"name":"highA","source_index":2,"target_index":1,"probability":0.3},
        {"name":"highB","source_index":2,"target_index":1,"probability":0.7},
        {"name":"low","source_index":1,"target_index":0,"probability":0.8}],
        "metadata":{"branching":[0,0,1],"conversion_coefficients":{"highA":1,"highB":0,"low":3}}})");parallel.close();
    import(parallel.fileName(),"Replace current quiver?",QMessageBox::Yes);
    coincidenceA->setCurrentText("highA");coincidenceB->setCurrentText("low");button(&window,"Calculate Coincidence Probability")->click();
    assert(coincidenceResult->text().startsWith("Physical: 0.24\nGamma emission: 0.03"));

    window.close();
    std::cout << "PASS: GUI construction, levels, transition, scene, path/vector, JSON import/export and failed-import rollback\n";
}
