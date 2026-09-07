#include "QuiverView.h"

#include <QPainter>
#include <QFontMetrics>
#include <cmath>

#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>

#include "COINAlgebra/DecayQuiver.h"
#include "COINAlgebra/DecayLevel.h"
#include "COINAlgebra/DecayTransition.h"

#include <QPolygonF>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <map>

QuiverView::QuiverView(QWidget* parent)
    : QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));
    setRenderHint(QPainter::Antialiasing);
}

void QuiverView::setQuiver(DecayQuiver* q)
{
    fQuiver = q;
    refresh();
}

void QuiverView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    refresh();
}

void QuiverView::refresh()
{
    QGraphicsScene* s = scene();
    if (!s) return;
    s->clear();
    if (!fQuiver) return;

    const auto& levels = fQuiver->GetLevels();
    const auto& transitions = fQuiver->GetTransitions();
    const int n = static_cast<int>(levels.size());
    if (n == 0) return;

    // compute layout in view coordinates
    const double w = std::max(200.0, static_cast<double>(viewport()->width()));
    const double h = std::max(200.0, static_cast<double>(viewport()->height()));
    // Layout as decay scheme: horizontal level lines stacked vertically
    const double marginX = w * 0.12;
    const double startX = marginX;
    const double endX = w - marginX;
    const double availableH = h - 40.0; // top/bottom margin
    const double spacing = (n > 1) ? (availableH / (n-1)) : 0.0;
    std::vector<QPointF> pos;
    if ((int)fPositions.size() == n) {
        for (int i=0;i<n;++i) {
            double y = fPositions[i].y();
            QPointF p((startX + endX) * 0.5, y);
            pos.push_back(p);
        }
    } else {
        for (int i=0;i<n;++i) {
            double y = 20.0 + i * spacing;
            QPointF p((startX + endX) * 0.5, y);
            pos.push_back(p);
        }
        fPositions = pos;
    }
    fLineStartX = startX;
    fLineEndX = endX;
    const double nodeRadius = fNodeRadius; // vertical half-gap used when connecting transitions
    fLineHalfHeight = 6.0;

    // add edges with arrowheads and offset labels
    QPen edgePen(QColor(80,80,80));
    edgePen.setWidth(3);
    QBrush arrowBrush(QColor(80,80,80));
    const double arrowSize = 12.0;
    const double labelOffset = 12.0; // pixels perpendicular to the edge
    QFont probFont = font();
    probFont.setPointSizeF(std::max(8.0, probFont.pointSizeF()-1));

    if ((int)fTransitionOffsets.size() != (int)transitions.size()) {
        fTransitionOffsets.assign(transitions.size(), 0.0);
    }
    if ((int)fTransitionPinned.size() != (int)transitions.size()) {
        fTransitionPinned.assign(transitions.size(), 0);
    }

    // group transitions by (sourceIndex, targetIndex)
    std::map<std::pair<int,int>, std::vector<int>> groups;
    for (int tr_i=0; tr_i<(int)transitions.size(); ++tr_i) {
        const auto* tr = transitions[tr_i];
        int si=-1, ti_idx=-1;
        for (int i=0;i<n;++i) {
            if (levels[i] == tr->GetSource()) si = i;
            if (levels[i] == tr->GetTarget()) ti_idx = i;
        }
        if (si<0 || ti_idx<0) continue;
        groups[{si, ti_idx}].push_back(tr_i);
    }

    // compute even spacing for each group, but do not overwrite offsets for transitions the user dragged
    const double groupSpacing = 18.0;
    for (const auto& kv : groups) {
        const auto& idxs = kv.second;
        int m = (int)idxs.size();
        for (int j=0;j<m;++j) {
            int ti = idxs[j];
            if (fTransitionPinned[ti]) continue; // preserve user offset
            double pos = (j - (m-1)/2.0) * groupSpacing;
            fTransitionOffsets[ti] = pos;
        }
    }

    for (int tr_i=0; tr_i<(int)transitions.size(); ++tr_i) {
        const auto* tr = transitions[tr_i];
        int si=-1, ti_idx=-1;
        for (int i=0;i<n;++i) {
            if (levels[i] == tr->GetSource()) si = i;
            if (levels[i] == tr->GetTarget()) ti_idx = i;
        }
        if (si<0 || ti_idx<0) continue;

        QPointF a = pos[si];
        QPointF b = pos[ti_idx];

        // apply a small horizontal offset based on transition index to reduce overlap
        double baseOffset = ((tr_i % 5) - 2) * 10.0;
        double offsetX = baseOffset + fTransitionOffsets[tr_i];
        a += QPointF(offsetX, 0);
        b += QPointF(offsetX, 0);

        // connect from just beyond the level line to just before the target line
        QPointF start = a + QPointF(0, (b.y() > a.y()) ? (fLineHalfHeight + 4.0) : -(fLineHalfHeight + 4.0));
        QPointF end = b - QPointF(0, (b.y() > a.y()) ? (fLineHalfHeight + 4.0) : -(fLineHalfHeight + 4.0));

        // main line
        s->addLine(start.x(), start.y(), end.x(), end.y(), edgePen);

        // compute arrow direction
        QPointF d = end - start;
        double L = std::hypot(d.x(), d.y());
        if (L > 1e-6) {
            QPointF u = d / L;
            QPointF perp(-u.y(), u.x());
            // arrowhead as triangle
            QPointF p1 = end;
            QPointF p2 = end - u*arrowSize + perp*(arrowSize*0.5);
            QPointF p3 = end - u*arrowSize - perp*(arrowSize*0.5);
            QPolygonF tri;
            tri << p1 << p2 << p3;
            s->addPolygon(tri, edgePen, arrowBrush)->setZValue(0.5);

                // probability label slightly offset perpendicular to the arrow
                QString label = QString::number(tr->GetProbability(), 'g', 3);
                QGraphicsTextItem* t = s->addText(label, probFont);
                t->setDefaultTextColor(QColor(30,30,30));
                QRectF tb = t->boundingRect();
                QPointF mid = (start + end) * 0.5;
                QPointF labelPos = mid + perp * labelOffset - QPointF(tb.width()/2.0, tb.height()/2.0);
                // draw white background rect so label never overlaps lines
                QRectF bgRect(labelPos, tb.size());
                bgRect.adjust(-4.0, -2.0, 4.0, 2.0);
                QGraphicsRectItem* bg = s->addRect(bgRect, QPen(Qt::NoPen), QBrush(Qt::white));
                bg->setZValue(0.95);
                t->setPos(labelPos);
                t->setZValue(1);
        }
    }

    // draw level lines with thicker stroke and distinct color
    QPen levelPen(QColor(150,30,30));
    levelPen.setWidth(4);
    QBrush levelBrush(Qt::NoBrush);
    QFont nameFont = font();
    nameFont.setBold(true);
    nameFont.setPointSizeF(std::max(10.0, nameFont.pointSizeF()+1));
    for (int i=0;i<n;++i) {
        const auto* lvl = levels[i];
        const QPointF p = pos[i];
        double y = p.y();
        QGraphicsLineItem* line = s->addLine(fLineStartX, y, fLineEndX, y, levelPen);
        line->setZValue(0);
        QString name = QString::fromStdString(lvl->GetName());
        QGraphicsTextItem* ti = s->addText(name, nameFont);
        ti->setDefaultTextColor(QColor(10,10,10));
        QRectF tb = ti->boundingRect();
        // place label to the left of the level line
        ti->setPos(fLineStartX - tb.width() - 8.0, y - tb.height()/2.0);
        ti->setZValue(1);
    }

    s->setSceneRect(0, 0, w, h);
}

void QuiverView::mousePressEvent(QMouseEvent* event)
{
    if (!fQuiver) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    QPointF sp = mapToScene(event->pos());

    // check levels (horizontal lines)
        for (int i=0;i<(int)fPositions.size();++i) {
            const QPointF& p = fPositions[i];
            double y = p.y();
            if (sp.x() >= fLineStartX - 6.0 && sp.x() <= fLineEndX + 6.0 && std::abs(sp.y() - y) <= (fLineHalfHeight + 6.0)) {
                // clicked level i: left-drag to move, right-click for menu
                if (event->button() == Qt::LeftButton) {
                    fDraggingLevel = i;
                    fLastMousePos = event->pos();
                    setCursor(Qt::ClosedHandCursor);
                    return;
                }
                DecayLevel* lvl = fQuiver->GetLevels()[i];
                QMenu menu;
                QAction* renameA = menu.addAction("Rename");
                QAction* delA = menu.addAction("Delete");
                QAction* act = menu.exec(event->globalPos());
                if (act == renameA) {
                    bool ok = false;
                    QString name = QInputDialog::getText(this, "Rename Level", "Name:", QLineEdit::Normal, QString::fromStdString(lvl->GetName()), &ok);
                    if (ok && !name.isEmpty()) {
                        lvl->SetName(name.toStdString());
                        refresh();
                        emit levelRenamed(i, name);
                    }
                }
                else if (act == delA) {
                    if (fQuiver->RemoveLevel(lvl)) {
                        refresh();
                        emit levelRenamed(i, QString());
                    }
                }
                return;
            }
    }

    // check transitions (distance to segment)
    const auto& levels = fQuiver->GetLevels();
    const auto& transitions = fQuiver->GetTransitions();
    if ((int)fTransitionOffsets.size() != (int)transitions.size()) {
        fTransitionOffsets.assign(transitions.size(), 0.0);
    }
    for (int ti=0; ti<(int)transitions.size(); ++ti) {
        const auto* tr = transitions[ti];
        int si=-1, ti_idx=-1;
        for (int j=0;j<(int)levels.size();++j) {
            if (levels[j] == tr->GetSource()) si = j;
            if (levels[j] == tr->GetTarget()) ti_idx = j;
        }
        if (si<0 || ti_idx<0) continue;
        QPointF a = fPositions[si];
        QPointF b = fPositions[ti_idx];
        // account for the per-transition horizontal offset used when drawing
        double baseOffset = ((ti % 5) - 2) * 10.0;
        double offsetX = baseOffset + fTransitionOffsets[ti];
        a += QPointF(offsetX, 0);
        b += QPointF(offsetX, 0);
        // shorten by node radius
        QPointF d = b - a;
        double L = std::hypot(d.x(), d.y());
        if (L <= 1e-6) continue;
        QPointF u = d / L;
        QPointF start = a + u * fNodeRadius;
        QPointF end = b - u * fNodeRadius;

        // project sp onto segment
        QPointF ap = sp - start;
        double proj = (ap.x()* (end.x()-start.x()) + ap.y()*(end.y()-start.y())) / ((end-start).x()*(end-start).x() + (end-start).y()*(end-start).y());
        if (proj < 0.0 || proj > 1.0) continue;
        QPointF closest = start + (end-start) * proj;
        double dist2 = std::pow(sp.x()-closest.x(),2) + std::pow(sp.y()-closest.y(),2);
            if (dist2 <= 12.0*12.0) {
                // left-drag to move transition horizontally; right-click for menu
                if (event->button() == Qt::LeftButton) {
                    fDraggingTransition = ti;
                    fLastMousePos = event->pos();
                    setCursor(Qt::ClosedHandCursor);
                    return;
                }
                // clicked transition: offer Edit / Delete
                QMenu menu;
                QAction* editA = menu.addAction("Edit properties");
                QAction* delA = menu.addAction("Delete");
                QAction* act = menu.exec(event->globalPos());
                if (act == editA) {
                    // pick new source
                    QStringList items;
                    for (const auto* L : levels) items << QString::fromStdString(L->GetName());
                    bool ok1=false;
                    QString srcName = QInputDialog::getItem(this, "Edit Transition - Source", "Source:", items, 0, false, &ok1);
                    if (!ok1) { return; }
                    bool ok2=false;
                    QString tgtName = QInputDialog::getItem(this, "Edit Transition - Target", "Target:", items, 0, false, &ok2);
                    if (!ok2) { return; }
                    bool ok3=false;
                    double p = QInputDialog::getDouble(this, "Edit Transition - Probability", "Probability:", tr->GetProbability(), 0.0, 1.0, 6, &ok3);
                    if (!ok3) { return; }
                    DecayLevel* src = fQuiver->GetLevel(srcName.toStdString());
                    DecayLevel* tgt = fQuiver->GetLevel(tgtName.toStdString());
                    if (src && tgt) {
                        const_cast<DecayTransition*>(tr)->SetSource(src);
                        const_cast<DecayTransition*>(tr)->SetTarget(tgt);
                        const_cast<DecayTransition*>(tr)->SetProbability(p);
                        refresh();
                        emit transitionEdited(ti, p);
                    }
                } else if (act == delA) {
                    if (fQuiver->RemoveTransition(tr)) {
                        refresh();
                        emit transitionEdited(ti, 0.0);
                    }
                }
                return;
            }
    }

    QGraphicsView::mousePressEvent(event);
}

void QuiverView::mouseMoveEvent(QMouseEvent* event)
{
    if (fDraggingLevel >= 0) {
        QPointF scenePos = mapToScene(event->pos());
        // clamp y
        double y = std::max(10.0, std::min(scenePos.y(), viewport()->height()-10.0));
        fPositions[fDraggingLevel].setY(y);
        refresh();
        return;
    }

    if (fDraggingTransition >= 0) {
        // compute delta in pixels horizontally
        QPointF delta = event->pos() - fLastMousePos;
        fLastMousePos = event->pos();
        fTransitionOffsets[fDraggingTransition] += delta.x();
        // mark this transition as user-adjusted so automatic spacing won't override it
        if (fDraggingTransition >= 0 && fDraggingTransition < (int)fTransitionPinned.size())
            fTransitionPinned[fDraggingTransition] = 1;
        refresh();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void QuiverView::mouseReleaseEvent(QMouseEvent* event)
{
    if (fDraggingLevel >= 0 || fDraggingTransition >= 0) {
        fDraggingLevel = -1;
        fDraggingTransition = -1;
        unsetCursor();
        emit transitionEdited(-1, 0.0);
        emit levelRenamed(-1, QString());
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}
