#include "QuiverView.h"
#include "TransitionEditor.h"
#include "StudioModel.h"
#include "ViewState.h"
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QJsonArray>
#include <QWheelEvent>
#include <QColorDialog>

#include <QPainter>
#include <QFontMetrics>
#include <cmath>

#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>

#include "COINAlgebra/Core/DecayQuiver.h"
#include "COINAlgebra/Core/DecayLevel.h"
#include "COINAlgebra/Core/DecayTransition.h"

#include <QPolygonF>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <algorithm>
#include <map>

QuiverView::QuiverView(QWidget* parent)
    : QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setBackgroundBrush(QColor("#f3f7fb"));
    setRenderHint(QPainter::Antialiasing);
}

void QuiverView::setQuiver(DecayQuiver* q)
{
    fTopLevel = nullptr;
    fHighlightedTransition = nullptr;
    fHighlightedLevel = nullptr;
    fPositions.clear();
    fTransitionOffsets.clear();
    fTransitionPinned.clear();
    fDraggingLevel = fDraggingTransition = -1;
    unsetCursor();
    fMetadata={};
    fMode=0; fEnergyScale=1; fDraggingGroup=-1;
    resetTransform();
    fQuiver = q;
    refresh();
}

void QuiverView::focusOnLevel(int index)
{
    if (!fQuiver || index < 0 || index >= static_cast<int>(fQuiver->GetLevels().size())) return;
    fTopLevel = fQuiver->GetLevels()[index];
    fPositions.clear();
    fTransitionPinned.assign(fQuiver->GetTransitions().size(), 0);
    refresh();
}

void QuiverView::showAllLevels()
{
    fTopLevel = nullptr;
    fPositions.clear();
    if (fQuiver) fTransitionPinned.assign(fQuiver->GetTransitions().size(), 0);
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
    if (std::find(levels.begin(), levels.end(), fHighlightedLevel) == levels.end())
        fHighlightedLevel = nullptr;
    if (std::find(transitions.begin(), transitions.end(), fHighlightedTransition) == transitions.end())
        fHighlightedTransition = nullptr;
    int n = static_cast<int>(levels.size());
    if (fTopLevel) {
        const auto top = std::find(levels.begin(), levels.end(), fTopLevel);
        if (top == levels.end()) fTopLevel = nullptr;
        else n = static_cast<int>(top - levels.begin()) + 1;
    }
    fVisibleLevelCount = n;
    if (n == 0) {
        fPositions.clear();
        auto* title = s->addText("Your next decay scheme starts here");
        title->setDefaultTextColor(QColor("#506880"));
        auto* hint = s->addText("Add a level or import a JSON quiver to begin.");
        hint->setDefaultTextColor(QColor("#70859a"));
        const double width = viewport()->width();
        const double height = viewport()->height();
        title->setPos((width - title->boundingRect().width()) / 2, height / 2 - 30);
        hint->setPos((width - hint->boundingRect().width()) / 2, height / 2);
        s->setSceneRect(0, 0, width, height);
        return;
    }

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
            // The first level is at the bottom; subsequent levels rise above it.
            double y = h - 20.0 - i * spacing;
            QPointF p((startX + endX) * 0.5, y);
            pos.push_back(p);
        }
        fPositions = pos;
    }
    fLeft.assign(n,startX); fRight.assign(n,endX); fVisible.assign(n,true);
    auto groups=Studio::layoutGroups(fMetadata);
    auto selection=Studio::selectGraph(*fQuiver,fMetadata,fMode==2);
    fEdgeVisible=selection.transitions;
    for(int i=0;i<n;++i) fVisible[i]=selection.levels[i];
    std::vector<int> columns(n,-1);
    fCollapsed.assign(n,-1); fGroupCenters.assign(groups.size(),QPointF());
    for(int g=0;g<groups.size();++g) for(auto id:groups[g].toObject()["levels"].toArray())
        if(id.toInt()>=0 && id.toInt()<n && columns[id.toInt()]<0) columns[id.toInt()]=g;
    for(int i=0;i<n;++i) {
        if(fMode==1) {
            double cell=(endX-startX)/(groups.size()+1.0);
            int column=columns[i]<0?groups.size():columns[i];
            auto group=columns[i]<0?QJsonObject{}:groups[columns[i]].toObject();
            double x=group.value("x").toDouble(startX+(column+.5)*cell);
            double width=group.value("width").toDouble(cell*.76);
            fLeft[i]=x-width/2; fRight[i]=x+width/2; pos[i].setX(x);
        }
        if(columns[i]>=0) {
            auto g=groups[columns[i]].toObject();
            if(g["hidden"].toBool()) fVisible[i]=false;
            else if(g["collapsed"].toBool()) fCollapsed[i]=columns[i];
        }
    }
    try {
        auto coordinates=Studio::energyCoordinates(*fQuiver,fMetadata,fVisible);
        auto energyMode=fMetadata["view"].toObject()["energy"].toObject().value("mode").toString("auto");
        // Retain manual vertical arrangements for legacy, uncalibrated schemes.
        bool known=true;
        for(int i=0;i<n;++i) if(fVisible[i]) try { Studio::levelEnergy(*fQuiver,fMetadata,i); } catch(const std::exception&) {known=false;}
        if(known || energyMode!="auto" || fMode==2 || fEnergyScale!=1)
            for(int i=0;i<n;++i) pos[i].setY(h-20-coordinates[i]*availableH*fEnergyScale);
    } catch(const std::exception& error) {
        auto* warning=s->addText(QString::fromUtf8(error.what())); warning->setDefaultTextColor(Qt::darkRed); warning->setPos(startX,-55);
    }
    auto manual=fMetadata["view"].toObject()["level_y"].toObject();
    if(fMetadata["view"].toObject()["energy"].toObject().value("mode").toString("auto")=="auto")
        for(int i=0;i<n;++i) if(manual.contains(QString::number(i))) {
            bool calibrated=true;try {Studio::levelEnergy(*fQuiver,fMetadata,i);}catch(const std::exception&){calibrated=false;}
            if(!calibrated)pos[i].setY(manual[QString::number(i)].toDouble());
        }
    for(int g=0;g<groups.size();++g) {
        auto group=groups[g].toObject(); double cell=(endX-startX)/(groups.size()+1.0);
        double x=group.value("x").toDouble(startX+(g+.5)*cell), y=0; int count=0;
        for(int i=0;i<n;++i) if(columns[i]==g && fVisible[i]) {y+=pos[i].y();++count;}
        fGroupCenters[g]=QPointF(x,count?y/count:h/2);
        if(group["hidden"].toBool() || !count) continue;
        if(group["collapsed"].toBool()) {
            QPointF center=fGroupCenters[g];
            auto* box=s->addRect(QRectF(center.x()-75,center.y()-24,150,48),QPen(QColor(group.value("color").toString("#0f8b80"))),QBrush(Qt::white));
            box->setData(1,g); box->setData(2,"summary"); box->setZValue(2);
            auto* title=s->addText(group["name"].toString()+QString("\n%1 visible levels").arg(count));
            title->setPos(center.x()-70,center.y()-23);title->setData(1,g);title->setZValue(3);
            title->setToolTip("Drag horizontally to move; right-click to expand.");
        } else if(fMode==1) {
            auto* title=s->addText(group["name"].toString());
            title->setDefaultTextColor(QColor(group.value("color").toString("#0f8b80")));
            title->setPos(x-title->boundingRect().width()/2,-35);title->setData(1,g);
            title->setToolTip("Drag to move band; right-click to collapse.");
        }
    }
    fPositions = pos; // Keep hit testing aligned after a horizontal resize.
    fLineStartX = startX;
    fLineEndX = endX;
    const double nodeRadius = fNodeRadius; // vertical half-gap used when connecting transitions
    fLineHalfHeight = 6.0;

    // add edges with arrowheads and offset labels
    QPen edgePen(QColor("#647b91"));
    edgePen.setWidth(3);
    QBrush arrowBrush(QColor("#647b91"));
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

    auto savedOffsets=fMetadata["view"].toObject()["transition_offsets"].toObject();
    for(std::size_t i=0;i<transitions.size();++i) {
        QString name=QString::fromStdString(transitions[i]->GetName());
        if(savedOffsets.contains(name) && fDraggingTransition!=int(i)) {fTransitionOffsets[i]=savedOffsets[name].toDouble();fTransitionPinned[i]=1;}
    }
    // Place upper-source transitions first from left to right. Keep the input
    // order within a source level and leave quiver/path identities unchanged.
    std::map<const DecayLevel*, double> sourceY;
    for (int i = 0; i < n; ++i) if(fVisible[i]) sourceY[levels[i]] = pos[i].y();
    std::vector<std::size_t> laneOrder;
    for (std::size_t i = 0; i < transitions.size(); ++i)
        if (fEdgeVisible[i] && sourceY.count(transitions[i]->GetSource()) && sourceY.count(transitions[i]->GetTarget()))
            laneOrder.push_back(i);
    std::stable_sort(laneOrder.begin(), laneOrder.end(), [&](std::size_t a, std::size_t b) {
        return sourceY.at(transitions[a]->GetSource()) < sourceY.at(transitions[b]->GetSource());
    });
    // Give each transition a lane across the usable width, preserving drags.
    const double laneSpacing = (endX - startX) / (laneOrder.size() + 1.0);
    const double centerX = (startX + endX) * 0.5;
    for (std::size_t lane = 0; lane < laneOrder.size(); ++lane) {
        const auto i = laneOrder[lane];
        if (!fTransitionPinned[i])
            fTransitionOffsets[i] = fMode==1
                ? ((lane+1.0)/(laneOrder.size()+1.0)-.5)*(endX-startX)/(groups.size()+1.0)*.7
                : startX + (lane + 1) * laneSpacing - centerX;
    }

    std::map<std::pair<int,int>,std::vector<int>> aggregated;
    for (int tr_i=0; tr_i<(int)transitions.size(); ++tr_i) {
        const auto* tr = transitions[tr_i];
        int si=-1, ti_idx=-1;
        for (int i=0;i<n;++i) {
            if (levels[i] == tr->GetSource()) si = i;
            if (levels[i] == tr->GetTarget()) ti_idx = i;
        }
        if (si<0 || ti_idx<0 || !fVisible[si] || !fVisible[ti_idx] || !fEdgeVisible[tr_i]) continue;
        if(fCollapsed[si]>=0 || fCollapsed[ti_idx]>=0) {
            if(fCollapsed[si]>=0 && fCollapsed[si]==fCollapsed[ti_idx]) continue;
            int a=fCollapsed[si]>=0?-fCollapsed[si]-1:si;
            int b=fCollapsed[ti_idx]>=0?-fCollapsed[ti_idx]-1:ti_idx;
            aggregated[{a,b}].push_back(tr_i); continue;
        }

        QPointF a = pos[si];
        QPointF b = pos[ti_idx];

        const double offsetX = fMode==1 ? std::clamp(fTransitionOffsets[tr_i],-std::min(fRight[si]-fLeft[si],fRight[ti_idx]-fLeft[ti_idx])*.45,std::min(fRight[si]-fLeft[si],fRight[ti_idx]-fLeft[ti_idx])*.45) : fTransitionOffsets[tr_i];
        a += QPointF(offsetX, 0);
        b += QPointF(offsetX, 0);

        // connect from just beyond the level line to just before the target line
        QPointF start = a + QPointF(0, (b.y() > a.y()) ? (fLineHalfHeight + 4.0) : -(fLineHalfHeight + 4.0));
        QPointF end = b - QPointF(0, (b.y() > a.y()) ? (fLineHalfHeight + 4.0) : -(fLineHalfHeight + 4.0));

        const bool highlighted = tr == fHighlightedTransition || tr->GetSource() == fHighlightedLevel;
        QString groupId=columns[si]>=0 && columns[si]==columns[ti_idx]?groups[columns[si]].toObject()["id"].toString():QString();
        auto style=Studio::objectStyle(fMetadata,groupId,QString::fromStdString(tr->GetName()),true);
        QPen transitionPen(QColor(style["color"].toString())); transitionPen.setWidthF(style["line_width"].toDouble());
        transitionPen.setStyle(style["line_style"]=="dash"?Qt::DashLine:style["line_style"]=="dot"?Qt::DotLine:Qt::SolidLine);
        if (highlighted) { transitionPen.setColor(QColor("#d97706")); transitionPen.setWidth(4); }
        const auto tag = [&](QGraphicsItem* item, double z) {
            item->setData(0, tr_i);
            if(selection.contextTransitions[tr_i]) item->setOpacity(.35);
            item->setToolTip(QString::fromStdString(tr->GetName()));
            item->setZValue(highlighted ? z + 3 : z);
        };
        // All visual parts carry the same transition identity for picking.
        auto* shaft = s->addLine(start.x(), start.y(), end.x(), end.y(), transitionPen);
        tag(shaft, 0.2);

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
            tag(s->addPolygon(tri, transitionPen, QBrush(transitionPen.color())), 0.5);

                if(!style["labels"].toBool()) continue;
                // probability label slightly offset perpendicular to the arrow
                QString label = QString::number(tr->GetProbability(), 'g', 3);
                QGraphicsTextItem* t = s->addText(label, probFont);
                t->setDefaultTextColor(QColor(highlighted ? "#92400e" : "#334d64"));
                QRectF tb = t->boundingRect();
                double labelFraction=.5;
                if(fMode==1) {
                    auto rank=std::find(laneOrder.begin(),laneOrder.end(),std::size_t(tr_i))-laneOrder.begin();
                    labelFraction=.2+.6*(rank+.5)/std::max(std::size_t(1),laneOrder.size());
                }
                QPointF mid = start+(end-start)*labelFraction;
                QPointF labelPos = mid + perp * labelOffset - QPointF(tb.width()/2.0, tb.height()/2.0);
                // draw white background rect so label never overlaps lines
                QRectF bgRect(labelPos, tb.size());
                bgRect.adjust(-4.0, -2.0, 4.0, 2.0);
                QGraphicsRectItem* bg = s->addRect(bgRect, QPen(Qt::NoPen), QBrush(QColor(highlighted ? "#fef3c7" : "#f3f7fb")));
                tag(bg, 0.95);
                t->setPos(labelPos);
                tag(t, 1);
        }
    }

    // Collapsed groups preserve directed external connectivity. Counts describe
    // visual edges, not summed physical probabilities.
    for(const auto& entry:aggregated) {
        const auto endpoint=[&](int key){return key<0?fGroupCenters[-key-1]:pos[key];};
        QPointF a=endpoint(entry.first.first), b=endpoint(entry.first.second);
        QPointF d=b-a;double length=std::hypot(d.x(),d.y());if(length<1)continue;
        QPointF u=d/length, perp(-u.y(),u.x());
        double boxRadius=1/std::max(std::abs(u.x())/75,std::abs(u.y())/24);
        QPointF start=a+u*(entry.first.first<0?boxRadius+4:10),end=b-u*(entry.first.second<0?boxRadius+4:10);
        QPen pen(QColor("#647b91"),3); auto* edge=s->addLine(QLineF(start,end),pen);edge->setData(2,"aggregate");
        QStringList names;for(int i:entry.second)names<<QString::fromStdString(transitions[i]->GetName());edge->setToolTip(names.join("\n"));
        QPolygonF arrow;arrow<<end<<end-u*12+perp*6<<end-u*12-perp*6;s->addPolygon(arrow,pen,QBrush(pen.color()));
        auto* label=s->addText(QString("%1 transition%2").arg(entry.second.size()).arg(entry.second.size()==1?"":"s"));label->setPos((start+end)/2+perp*12);label->setToolTip(names.join("\n"));
    }

    // draw level lines with thicker stroke and distinct color
    QPen levelPen(QColor("#0f8b80"));
    levelPen.setWidth(4);
    QBrush levelBrush(Qt::NoBrush);
    QFont nameFont = font();
    nameFont.setBold(true);
    nameFont.setPointSizeF(std::max(10.0, nameFont.pointSizeF()+1));
    for (int i=0;i<n;++i) {
        if(!fVisible[i] || fCollapsed[i]>=0) continue;
        const auto* lvl = levels[i];
        const QPointF p = pos[i];
        double y = p.y();
        auto style=Studio::objectStyle(fMetadata,columns[i]>=0?groups[columns[i]].toObject()["id"].toString():QString(),QString::number(i),false);
        QPen currentLevelPen(QColor(style["color"].toString()));currentLevelPen.setWidthF(style["line_width"].toDouble());
        currentLevelPen.setStyle(style["line_style"]=="dash"?Qt::DashLine:style["line_style"]=="dot"?Qt::DotLine:Qt::SolidLine);
        if (lvl == fHighlightedLevel) currentLevelPen.setColor(QColor("#d97706"));
        QGraphicsLineItem* line = s->addLine(fLeft[i], y, fRight[i], y, currentLevelPen);
        line->setZValue(0);
        if(selection.contextLevels[i]) line->setOpacity(.35);
        if(!style["labels"].toBool()) continue;
        QString name = QString::fromStdString(lvl->GetName());
        QGraphicsTextItem* ti = s->addText(name, nameFont);
        ti->setDefaultTextColor(QColor("#233c53"));
        QRectF tb = ti->boundingRect();
        // place label to the left of the level line
        ti->setPos(fMode==1 ? fLeft[i] : fLeft[i]-tb.width()-8, fMode==1 ? y-tb.height()-3 : y-tb.height()/2);
        ti->setZValue(1);
        if(selection.contextLevels[i]) ti->setOpacity(.35);
    }

    if(std::none_of(fVisible.begin(),fVisible.end(),[](bool v){return v;})) {
        auto* text=s->addText("No levels in this focus or energy window.");text->setPos(30,50);
    }
    s->setSceneRect(s->itemsBoundingRect().adjusted(-30,-30,30,30));
}

void QuiverView::mousePressEvent(QMouseEvent* event)
{
    if (!fQuiver) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    QPointF sp = mapToScene(event->pos());

    for(auto* item:scene()->items(sp)) if(item->data(1).isValid()) {
        int g=item->data(1).toInt();
        if(event->button()==Qt::LeftButton) {fDraggingGroup=g;fLastMousePos=event->pos();setCursor(Qt::ClosedHandCursor);return;}
        if(event->button()==Qt::RightButton) {
            auto groups=Studio::layoutGroups(fMetadata);auto group=groups[g].toObject();
            QMenu menu;auto* toggle=menu.addAction(group["collapsed"].toBool()?"Expand group":"Collapse group");
            if(menu.exec(event->globalPos())==toggle) {auto view=fMetadata["view"].toObject();auto layouts=view["group_layout"].toObject();auto l=layouts[group["id"].toString()].toObject();l["collapsed"]=!group["collapsed"].toBool();layouts[group["id"].toString()]=l;view["group_layout"]=layouts;fMetadata["view"]=view;persistView();refresh();}
            return;
        }
    }
    // Pick the actual drawn label/arrow first, including labels over levels.
    if (event->button() == Qt::LeftButton) {
        for (auto* item : scene()->items(sp)) {
            if (!item->data(0).isValid()) continue;
            const int index = item->data(0).toInt();
            fHighlightedTransition = fQuiver->GetTransitions().at(index);
            fHighlightedLevel = nullptr;
            fDraggingTransition = index;
            fLastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor);
            refresh();
            return;
        }
        fHighlightedTransition = nullptr;
        fHighlightedLevel = nullptr;
        refresh();
    }

    // check levels (horizontal lines)
        for (int i=0;i<(int)fPositions.size();++i) {
            const QPointF& p = fPositions[i];
            double y = p.y();
            if (fVisible[i] && fCollapsed[i]<0 && sp.x() >= fLeft[i] - 6.0 && sp.x() <= fRight[i] + 6.0 && std::abs(sp.y() - y) <= (fLineHalfHeight + 6.0)) {
                // clicked level i: left-drag to move, right-click for menu
                if (event->button() == Qt::LeftButton) {
                    fHighlightedLevel = fQuiver->GetLevels()[i];
                    fDraggingLevel = i;
                    fLastMousePos = event->pos();
                    setCursor(Qt::ClosedHandCursor);
                    refresh();
                    return;
                }
                DecayLevel* lvl = fQuiver->GetLevels()[i];
                QMenu menu;
                QAction* focusA = menu.addAction("Zoom: make this the top level");
                QAction* allA = menu.addAction("Show all levels");
                allA->setEnabled(fTopLevel != nullptr);
                menu.addSeparator();
                QAction* colorA = menu.addAction("Style…");
                QAction* renameA = menu.addAction("Rename");
                QAction* delA = menu.addAction("Delete");
                QAction* act = menu.exec(event->globalPos());
                if (act == focusA) {
                    focusOnLevel(i);
                } else if (act == allA) {
                    showAllLevels();
                } else if (act == colorA) {
                    setObjectStyle(i,false);
                } else if (act == renameA) {
                    bool ok = false;
                    QString name = QInputDialog::getText(this, "Rename Level", "Name:", QLineEdit::Normal, QString::fromStdString(lvl->GetName()), &ok);
                    if (ok && !name.isEmpty()) {
                        lvl->SetName(name.toStdString());
                        refresh();
                        emit levelRenamed(i, name);
                    }
                }
                else if (act == delA) {
                    emit levelDeleteRequested(i);
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
        for (int j=0;j<fVisibleLevelCount;++j) {
            if (levels[j] == tr->GetSource()) si = j;
            if (levels[j] == tr->GetTarget()) ti_idx = j;
        }
        if (si<0 || ti_idx<0 || !fVisible[si] || !fVisible[ti_idx] || !fEdgeVisible[ti] || fCollapsed[si]>=0 || fCollapsed[ti_idx]>=0) continue;
        QPointF a = fPositions[si];
        QPointF b = fPositions[ti_idx];
        // account for the per-transition horizontal offset used when drawing
        const double offsetX = fMode==1 ? std::clamp(fTransitionOffsets[ti],-(fRight[si]-fLeft[si])*.45,(fRight[si]-fLeft[si])*.45) : fTransitionOffsets[ti];
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
                    fHighlightedTransition = tr;
                    fDraggingTransition = ti;
                    refresh();
                    fLastMousePos = event->pos();
                    setCursor(Qt::ClosedHandCursor);
                    return;
                }
                // clicked transition: offer Edit / Delete
                QMenu menu;
                QAction* editA = menu.addAction("Edit properties");
                QAction* colorA = menu.addAction("Style…");
                QAction* delA = menu.addAction("Delete");
                QAction* act = menu.exec(event->globalPos());
                if (act == editA) {
                    if (Studio::editTransition(this, *transitions[ti], levels)) {
                        refresh();
                        emit transitionEdited(ti, transitions[ti]->GetProbability());
                    }
                } else if (act == colorA) {
                    setObjectStyle(ti,true);
                } else if (act == delA) {
                    if (fQuiver->RemoveTransition(tr)) {
                        refresh();
                        emit transitionEdited(ti, 0.0);
                    }
                }
                return;
            }
    }

    if (event->button() == Qt::RightButton && fTopLevel) {
        QMenu menu;
        auto* allA = menu.addAction("Show all levels");
        if (menu.exec(event->globalPos()) == allA) showAllLevels();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void QuiverView::mouseMoveEvent(QMouseEvent* event)
{
    if(fDraggingGroup>=0) {
        auto groups=Studio::layoutGroups(fMetadata); if(fDraggingGroup>=groups.size()) return;
        auto id=groups[fDraggingGroup].toObject()["id"].toString();auto view=fMetadata["view"].toObject();auto layouts=view["group_layout"].toObject();auto l=layouts[id].toObject();
        double dx=mapToScene(event->pos()).x()-mapToScene(fLastMousePos.toPoint()).x();fLastMousePos=event->pos();
        l["x"]=fGroupCenters[fDraggingGroup].x()+dx;layouts[id]=l;view["group_layout"]=layouts;fMetadata["view"]=view;refresh();return;
    }
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
        QPointF delta = mapToScene(event->pos()) - mapToScene(fLastMousePos.toPoint());
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
    if(fDraggingGroup>=0) {fDraggingGroup=-1;unsetCursor();persistView();return;}
    if (fDraggingLevel >= 0 || fDraggingTransition >= 0) {
        auto state=fMetadata["view"].toObject();
        if(fDraggingLevel>=0) {auto ys=state["level_y"].toObject();ys[QString::number(fDraggingLevel)]=fPositions[fDraggingLevel].y();state["level_y"]=ys;}
        if(fDraggingTransition>=0) {auto offsets=state["transition_offsets"].toObject();offsets[QString::fromStdString(fQuiver->GetTransitions()[fDraggingTransition]->GetName())]=fTransitionOffsets[fDraggingTransition];state["transition_offsets"]=offsets;}
        fMetadata["view"]=state;
        fDraggingLevel = -1;
        fDraggingTransition = -1;
        persistView();
        unsetCursor();
        emit transitionEdited(-1, 0.0);
        emit levelRenamed(-1, QString());
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void QuiverView::wheelEvent(QWheelEvent* event) {
    double factor=event->angleDelta().y()>0?1.15:1/1.15;
    double zoom=transform().m11()*factor;
    if(zoom>=.1 && zoom<=20) scale(factor,factor);
    event->accept();
}

void QuiverView::setMetadata(const QJsonObject& metadata) {
    auto normalized=Studio::normalizeViewMetadata(metadata);
    if(normalized!=fMetadata) fPositions.clear();
    fMetadata=normalized;
    auto state=fMetadata["view"].toObject();
    fMode=state.value("mode").toInt(fMode);
    fEnergyScale=state.value("energy_scale").toDouble(fEnergyScale);
}
QJsonObject QuiverView::viewState() const {
    auto state=fMetadata["view"].toObject();state["mode"]=fMode;state["energy_scale"]=fEnergyScale;
    auto center=mapToScene(viewport()->rect().center());
    state["camera"]=QJsonObject{{"x",center.x()},{"y",center.y()},{"zoom",transform().m11()}};
    if(fQuiver) {
        const auto& levels=fQuiver->GetLevels();auto level=std::find(levels.begin(),levels.end(),fHighlightedLevel);
        if(level!=levels.end())state["highlight_level"]=int(level-levels.begin());else state.remove("highlight_level");
        if(fHighlightedTransition && std::find(fQuiver->GetTransitions().begin(),fQuiver->GetTransitions().end(),fHighlightedTransition)!=fQuiver->GetTransitions().end())state["highlight_transition"]=QString::fromStdString(fHighlightedTransition->GetName());else state.remove("highlight_transition");
    }
    return state;
}
void QuiverView::persistView() {emit appearanceChanged(fMetadata);}
void QuiverView::configureView(const QJsonObject& state) {
    fMetadata["view"]=state;fMode=state.value("mode").toInt(fMode);fEnergyScale=state.value("energy_scale").toDouble(fEnergyScale);
    fHighlightedLevel=nullptr;fHighlightedTransition=nullptr;
    if(fQuiver) {
        int i=state.value("highlight_level").toInt(-1);if(i>=0 && i<int(fQuiver->GetLevels().size()))fHighlightedLevel=fQuiver->GetLevels()[i];
        fHighlightedTransition=fQuiver->GetTransition(state.value("highlight_transition").toString().toStdString());
        fTransitionPinned.assign(fQuiver->GetTransitions().size(),0);
    }
    fTopLevel=nullptr;fPositions.clear();refresh();persistView();
}
void QuiverView::restoreView(const QJsonObject& state) {
    configureView(state);resetTransform();auto camera=state["camera"].toObject();
    if(camera.isEmpty()) fitInView(sceneRect(),Qt::KeepAspectRatio);
    else {scale(camera["zoom"].toDouble(),camera["zoom"].toDouble());centerOn(camera["x"].toDouble(),camera["y"].toDouble());}
}
void QuiverView::setMode(int mode) {
    auto state=viewState();state["mode"]=mode;configureView(state);fitInView(sceneRect(),Qt::KeepAspectRatio);
}
void QuiverView::setEnergyScale(double scale) {
    auto state=viewState();state["energy_scale"]=scale;configureView(state);
}
void QuiverView::setObjectStyle(int index,bool transition) {
    const QString key=transition?"transition_styles":"level_styles";
    const QString id=transition?QString::fromStdString(fQuiver->GetTransitions()[index]->GetName()):QString::number(index);
    auto state=fMetadata["view"].toObject();auto styles=state[key].toObject();auto old=styles[id].toObject();
    QDialog dialog(this);dialog.setWindowTitle("Object style");auto* form=new QFormLayout(&dialog);
    auto* color=new QLineEdit(old.value("color").toString());color->setPlaceholderText("Blank inherits group/global colour");form->addRow("Colour (#rrggbb)",color);
    auto* width=new QDoubleSpinBox();width->setRange(0,20);width->setSpecialValueText("Inherit");width->setValue(old["line_width"].toDouble());form->addRow("Line width",width);
    auto* dash=new QComboBox();dash->addItems({"inherit","solid","dash","dot"});dash->setCurrentText(old.value("line_style").toString("inherit"));form->addRow("Line style",dash);
    auto* labels=new QComboBox();labels->addItems({"Inherit","Show","Hide"});labels->setCurrentIndex(old.contains("labels")?(old["labels"].toBool()?1:2):0);form->addRow("Labels",labels);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{if(color->text().isEmpty() || QColor(color->text()).isValid())dialog.accept();});connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    if(dialog.exec()!=QDialog::Accepted)return;
    QJsonObject style;if(!color->text().isEmpty())style["color"]=color->text();if(width->value()>0)style["line_width"]=width->value();if(dash->currentIndex())style["line_style"]=dash->currentText();if(labels->currentIndex())style["labels"]=labels->currentIndex()==1;
    if(style.isEmpty())styles.remove(id);else styles[id]=style;state[key]=styles;fMetadata["view"]=state;refresh();persistView();
}
