#include "QuiverView.h"
#include "COINAlgebra/Core/DecayQuiver.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>
#include <cassert>
#include <cmath>
#include <iostream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    DecayQuiver quiver;
    QuiverView view;
    view.resize(800, 600);
    view.show();
    app.processEvents();
    view.setQuiver(&quiver);
    const auto levelY = [&](const QString& label) {
        for (auto* item : view.scene()->items())
            if (auto* text = dynamic_cast<QGraphicsTextItem*>(item))
                if (text->toPlainText() == label) return text->sceneBoundingRect().center().y();
        assert(false && "Missing level label");
        return 0.0;
    };
    auto* ground = quiver.AddLevel("ground");
    view.refresh();
    assert(levelY("ground") > view.viewport()->height() - 30);
    auto* middle = quiver.AddLevel("middle");
    view.refresh();
    assert(levelY("middle") < levelY("ground"));
    auto* upper = quiver.AddLevel("upper");
    view.refresh();
    assert(levelY("upper") < levelY("middle"));
    assert(levelY("middle") < levelY("ground"));
    auto* transition = quiver.AddTransition("gamma", upper, middle, 0.4);
    view.refresh();

    const auto edit = [&](bool accept) {
        QPointF midpoint;
        bool found = false;
        for (auto* item : view.scene()->items()) {
            auto* line = dynamic_cast<QGraphicsLineItem*>(item);
            if (line && std::abs(line->line().dy()) > 1) {
                midpoint = line->mapToScene(line->line().pointAt(0.5));
                found = true;
                break;
            }
        }
        assert(found);
        QTimer driver;
        int dialogs = 0;
        bool menuOpened = false;
        QObject::connect(&driver, &QTimer::timeout, [&] {
            if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget())) {
                if (!menuOpened) {
                    menuOpened = true;
                    menu->setActiveAction(menu->actions().front());
                    QKeyEvent key(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                    QApplication::sendEvent(menu, &key);
                }
            } else if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
                ++dialogs;
                assert(dialog->windowTitle() == "Edit Transition");
                const auto combos = dialog->findChildren<QComboBox*>();
                assert(combos.size() == 2);
                auto* source = dialog->findChild<QComboBox*>("transitionSource");
                auto* target = dialog->findChild<QComboBox*>("transitionTarget");
                auto* probability = dialog->findChild<QDoubleSpinBox*>("transitionProbability");
                assert(source && target && probability);
                assert(source->currentText() == QString::fromStdString(transition->GetSource()->GetName()));
                assert(target->currentText() == QString::fromStdString(transition->GetTarget()->GetName()));
                assert(std::abs(probability->value() - transition->GetProbability()) < 1e-12);
                source->setCurrentText(accept ? "middle" : "upper");
                target->setCurrentText(accept ? "ground" : "middle");
                probability->setValue(accept ? 0.75 : 0.2);
                if (accept && argc > 1) assert(dialog->grab().save(argv[1]));
                if (accept) dialog->accept(); else dialog->reject();
            }
        });
        driver.start(10);
        const QPoint position = view.mapFromScene(midpoint);
        QMouseEvent press(QEvent::MouseButtonPress, position, view.viewport()->mapToGlobal(position),
                          Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        QApplication::sendEvent(view.viewport(), &press);
        driver.stop();
        assert(menuOpened && dialogs == 1);
    };
    edit(true);
    assert(transition->GetSource() == middle);
    assert(transition->GetTarget() == ground);
    assert(transition->GetProbability() == 0.75);
    edit(false);
    assert(transition->GetSource() == middle);
    assert(transition->GetTarget() == ground);
    assert(transition->GetProbability() == 0.75);
    std::cout << "PASS: bottom-up levels and one transition editor with accept/cancel\n";
}
