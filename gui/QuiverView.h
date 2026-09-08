#pragma once

#include <QGraphicsView>
#include <QPointF>
#include <vector>

class DecayQuiver;
class DecayLevel;

class QuiverView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit QuiverView(QWidget* parent = nullptr);

    void setQuiver(DecayQuiver* q);

    // Rebuild the scene from the current quiver data.
    void refresh();
    void focusOnLevel(int index);
    void showAllLevels();

private:
    DecayQuiver* fQuiver = nullptr;
    const DecayLevel* fTopLevel = nullptr;
    int fVisibleLevelCount = 0;
    std::vector<QPointF> fPositions;
    double fNodeRadius = 28.0;
    double fLineStartX = 0.0;
    double fLineEndX = 0.0;
    double fLineHalfHeight = 6.0;
    // per-transition horizontal offsets (user-adjustable)
    std::vector<double> fTransitionOffsets;
    // whether a transition has been manually dragged by the user
    std::vector<char> fTransitionPinned;

    // drag state
    int fDraggingLevel = -1;
    int fDraggingTransition = -1;
    QPointF fLastMousePos;
signals:
    void levelRenamed(int index, const QString& newName);
    void transitionEdited(int index, double probability);
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
protected:
    void resizeEvent(QResizeEvent* event) override;
};
