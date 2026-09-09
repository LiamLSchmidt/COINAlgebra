#pragma once

#include <QGraphicsView>
#include <QPointF>
#include <QJsonObject>
#include <vector>

class DecayQuiver;
class DecayLevel;
class DecayTransition;

class QuiverView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit QuiverView(QWidget* parent = nullptr);

    void setQuiver(DecayQuiver* q);
    void setMetadata(const QJsonObject& metadata);
    QJsonObject viewState() const;
    void restoreView(const QJsonObject& state);
    void setMode(int mode);
    void setEnergyScale(double scale);
    void configureView(const QJsonObject& state);
    int mode() const { return fMode; }



    // Rebuild the scene from the current quiver data.
    void refresh();
    void focusOnLevel(int index);
    void showAllLevels();

private:
    QJsonObject fMetadata;
    int fMode=0;
    double fEnergyScale=1;
    std::vector<double> fLeft, fRight;
    std::vector<bool> fVisible, fEdgeVisible;
    std::vector<int> fCollapsed;
    std::vector<QPointF> fGroupCenters;
    int fDraggingGroup=-1;
    void setObjectStyle(int index, bool transition);
    void persistView();
    void wheelEvent(QWheelEvent* event) override;
    DecayQuiver* fQuiver = nullptr;
    const DecayLevel* fTopLevel = nullptr;
    const DecayTransition* fHighlightedTransition = nullptr;
    const DecayLevel* fHighlightedLevel = nullptr;
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
    void levelDeleteRequested(int index);
    void appearanceChanged(const QJsonObject& metadata);
    void levelRenamed(int index, const QString& newName);
    void transitionEdited(int index, double probability);
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
protected:
    void resizeEvent(QResizeEvent* event) override;
};
