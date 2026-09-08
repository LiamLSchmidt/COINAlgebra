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
#include <cassert>
#include <iostream>

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
    auto edits = window.findChildren<QLineEdit*>();
    assert(edits.size() == 2);
    for (const auto& name : {"d0", "d1", "d2"}) {
        edits[0]->setText(name);
        button(&window, "Add Level")->click();
    }
    auto combos = window.findChildren<QComboBox*>();
    assert(combos.size() == 2);
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
    assert(json["levels"].toArray().size() == 3);
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
    QFile invalid(output.path() + "/invalid.json");
    assert(invalid.open(QIODevice::WriteOnly));
    invalid.write(R"({"levels":["bad"],"transitions":[{"name":"broken","source_index":0,"target_index":99,"probability":1}]})");
    invalid.close();
    import(invalid.fileName(), "Import Failed", QMessageBox::Ok);
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
    if (argc > 1) assert(window.grab().save(argv[1]));
    window.close();
    std::cout << "PASS: GUI construction, levels, transition, scene, path/vector, JSON import/export and failed-import rollback\n";
}
