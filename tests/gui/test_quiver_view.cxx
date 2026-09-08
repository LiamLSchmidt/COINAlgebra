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
#include <algorithm>

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
    // More than five transitions must occupy distinct, evenly spaced lanes,
    // including parallel branches. Previously the x positions repeated modulo 5.
    for (int i = 0; i < 11; ++i)
        quiver.AddTransition("branch_" + std::to_string(i), upper, ground, 0.1);
    view.refresh();
    const auto lanes = [&] {
        std::vector<double> xs;
        for (auto* item : view.scene()->items())
            if (auto* line = dynamic_cast<QGraphicsLineItem*>(item))
                if (std::abs(line->line().dy()) > 1) xs.push_back(line->line().x1());
        std::sort(xs.begin(), xs.end());
        return xs;
    };
    const auto checkSpacing = [&] {
        const auto xs = lanes();
        assert(xs.size() == 12);
        const double gap = xs[1] - xs[0];
        assert(gap > 20);
        for (std::size_t i = 2; i < xs.size(); ++i)
            assert(std::abs(xs[i] - xs[i-1] - gap) < 1e-8);
        assert(xs.back() - xs.front() > view.viewport()->width() * 0.6);
        return gap;
    };
    const double before = checkSpacing();
    view.resize(1200, 600);
    app.processEvents();
    assert(checkSpacing() > before);

    // The first inserted transition starts at the middle level; it must be
    // placed after all upper-source branches, regardless of insertion order.
    std::vector<std::pair<double, double>> arrowSources;
    for (auto* item : view.scene()->items())
        if (auto* line = dynamic_cast<QGraphicsLineItem*>(item))
            if (std::abs(line->line().dy()) > 1)
                arrowSources.emplace_back(line->line().x1(), line->line().y1());
    std::sort(arrowSources.begin(), arrowSources.end());
    for (std::size_t i = 0; i + 1 < arrowSources.size(); ++i)
        assert(std::abs(arrowSources[i].second - levelY("upper") - 10) < 1e-8);
    assert(std::abs(arrowSources.back().second - levelY("middle") - 10) < 1e-8);
    assert(quiver.GetTransitions().front() == transition);

    // Hit testing after resizing must find the arrow in its new lane.
    const double x = lanes().front();
    const double y = (levelY("middle") + levelY("ground")) / 2;
    const QPoint from = view.mapFromScene(QPointF(x, y));
    const QPoint to = from + QPoint(15, 0);
    QMouseEvent press(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &press);
    QMouseEvent move(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &move);
    QMouseEvent release(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &release);
    assert(std::abs(lanes().front() - x - 15) < 1e-8);
    view.refresh();
    assert(std::abs(lanes().front() - x - 15) < 1e-8);
    // Use the actual level menu to focus on middle, hiding upper and its
    // transitions without changing the underlying quiver.
    QTimer zoomDriver;
    bool zoomChosen = false;
    QObject::connect(&zoomDriver, &QTimer::timeout, [&] {
        auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!menu || zoomChosen) return;
        zoomChosen = true;
        assert(menu->actions().front()->text() == "Zoom: make this the top level");
        menu->setActiveAction(menu->actions().front());
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(menu, &key);
    });
    zoomDriver.start(10);
    const QPoint levelPoint = view.mapFromScene(QPointF(view.viewport()->width()/2, levelY("middle")));
    QMouseEvent zoomPress(QEvent::MouseButtonPress, levelPoint, view.viewport()->mapToGlobal(levelPoint),
                         Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &zoomPress);
    zoomDriver.stop();
    assert(zoomChosen);
    assert(levelY("middle") < 30);
    assert(lanes().size() == 1);
    assert(quiver.GetLevels().size() == 3 && quiver.GetTransitions().size() == 12);
    for (auto* item : view.scene()->items())
        if (auto* text = dynamic_cast<QGraphicsTextItem*>(item))
            assert(text->toPlainText() != "upper");
    view.resize(1000, 650);
    app.processEvents();
    assert(lanes().size() == 1);
    view.showAllLevels();
    assert(lanes().size() == 12);
    assert(levelY("upper") < levelY("middle"));
    view.focusOnLevel(0);
    assert(lanes().empty());
    view.setQuiver(&quiver);
    assert(lanes().size() == 12);
    const auto click = [&](QPointF point) {
        auto p = view.mapFromScene(point);
        QMouseEvent down(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(view.viewport(), &down);
        QMouseEvent up(QEvent::MouseButtonRelease, p, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(view.viewport(), &up);
    };
    const auto part = [&](int index, bool label) -> QGraphicsItem* {
        for (auto* item : view.scene()->items()) {
            if (!item->data(0).isValid() || item->data(0).toInt() != index) continue;
            if (label && dynamic_cast<QGraphicsTextItem*>(item)) return item;
            if (!label && dynamic_cast<QGraphicsLineItem*>(item)) return item;
        }
        assert(false); return nullptr;
    };
    click(part(0, true)->sceneBoundingRect().center());
    assert(dynamic_cast<QGraphicsLineItem*>(part(0, false))->pen().color() == QColor("#d97706"));
    assert(dynamic_cast<QGraphicsTextItem*>(part(0, true))->defaultTextColor() == QColor("#92400e"));
    auto* arrow = dynamic_cast<QGraphicsLineItem*>(part(1, false));
    click(arrow->mapToScene(arrow->line().pointAt(0.2)));
    assert(dynamic_cast<QGraphicsTextItem*>(part(1, true))->defaultTextColor() == QColor("#92400e"));
    assert(dynamic_cast<QGraphicsLineItem*>(part(0, false))->pen().color() == QColor("#647b91"));
    click(QPointF(5, 5));
    assert(dynamic_cast<QGraphicsLineItem*>(part(1, false))->pen().color() == QColor("#647b91"));
    click(QPointF(view.viewport()->width() * 0.85, levelY("upper")));
    for (int i = 1; i < 12; ++i) {
        assert(dynamic_cast<QGraphicsLineItem*>(part(i, false))->pen().color() == QColor("#d97706"));
        assert(dynamic_cast<QGraphicsTextItem*>(part(i, true))->defaultTextColor() == QColor("#92400e"));
    }
    assert(dynamic_cast<QGraphicsLineItem*>(part(0, false))->pen().color() == QColor("#647b91"));
    click(QPointF(view.viewport()->width() * 0.85, levelY("middle")));
    assert(dynamic_cast<QGraphicsLineItem*>(part(0, false))->pen().color() == QColor("#d97706"));
    assert(dynamic_cast<QGraphicsLineItem*>(part(1, false))->pen().color() == QColor("#647b91"));
    click(part(1, true)->sceneBoundingRect().center());
    assert(dynamic_cast<QGraphicsLineItem*>(part(0, false))->pen().color() == QColor("#647b91"));
    click(QPointF(view.viewport()->width() * 0.85, levelY("ground")));
    for (int i = 0; i < 12; ++i)
        assert(dynamic_cast<QGraphicsLineItem*>(part(i, false))->pen().color() == QColor("#647b91"));
    std::cout << "PASS: level layout, transition editing, even lanes, resize and dragging\n";
}
