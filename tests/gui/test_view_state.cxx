#include "ViewState.h"
#include "StudioModel.h"
#include "QuiverView.h"
#include "QuiverJson.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include <cassert>
#include <cmath>
#include <iostream>

int main(int argc,char** argv) {
    QApplication app(argc,argv);
    auto document=Studio::readJson(R"({"levels":["ground","middle","upper","external"],"transitions":[
        {"name":"a","source_index":2,"target_index":1,"probability":0.5},
        {"name":"parallel","source_index":2,"target_index":1,"probability":0.5},
        {"name":"b","source_index":1,"target_index":0,"probability":1},
        {"name":"in","source_index":3,"target_index":2,"probability":1}]})");
    auto& q=*document.quiver;
    QJsonObject metadata{{"groups",QJsonArray{
        QJsonObject{{"id","band"},{"name","Band A"},{"kind","band"},{"levels",QJsonArray{1,2}}}}},
        {"subquivers",QJsonArray{QJsonObject{{"id","selected"},{"name","One branch"},{"levels",QJsonArray{1,2}},{"transitions",QJsonArray{"a"}}}}},
        {"level_energies_keV",QJsonObject{{"0",0},{"1",100},{"2",1000},{"3",2000}}}};
    for(int i=0;i<4;++i) q.GetLevels()[i]->SetEnergy(metadata["level_energies_keV"].toObject()[QString::number(i)].toDouble());
    metadata.remove("level_energies_keV");
    auto state=QJsonObject{{"mode",2},{"focus",QJsonObject{{"type","subquiver"},{"id","selected"}}},
        {"group_layout",QJsonObject{{"band",QJsonObject{{"color","#ff0000"},{"x",300},{"width",180}}}}}};
    metadata["view"]=state;Studio::validateViewMetadata(q,metadata);
    auto projection=Studio::selectGraph(q,metadata,true);
    assert(projection.levels==std::vector<bool>({false,true,true,false}));
    assert(projection.transitions==std::vector<bool>({true,false,false,false}));
    // Explicit arrow membership excludes a parallel edge with the same endpoints.
    auto f=state["focus"].toObject();f["context"]="incoming";f["depth"]=1;state["focus"]=f;metadata["view"]=state;
    projection=Studio::selectGraph(q,metadata,true);
    assert(projection.levels[3] && !projection.levels[0] && projection.contextLevels[3]);
    assert(projection.transitions[3] && !projection.transitions[1]);
    f["context"]="descendants";state["focus"]=f;metadata["view"]=state;
    projection=Studio::selectGraph(q,metadata,true);assert(projection.levels[0] && !projection.levels[3]);
    f=QJsonObject{{"type","level"},{"level",0},{"context","ancestors"}};state["focus"]=f;metadata["view"]=state;
    projection=Studio::selectGraph(q,metadata,true);for(bool v:projection.levels)assert(v);
    state["energy"]=QJsonObject{{"mode","physical"}};metadata["view"]=state;
    std::vector<bool> visible(4,true);auto y=Studio::energyCoordinates(q,metadata,visible);assert(std::abs(y[1]-.05)<1e-12);
    state["energy"]=QJsonObject{{"mode","compressed"},{"compression_keV",100}};metadata["view"]=state;
    y=Studio::energyCoordinates(q,metadata,visible);assert(y[1]>.05 && y[1]<y[2]);
    state["energy"]=QJsonObject{{"mode","uniform"}};metadata["view"]=state;
    y=Studio::energyCoordinates(q,metadata,visible);assert(std::abs(y[1]-1./3)<1e-12);
    state["energy"]=QJsonObject{{"mode","physical"},{"window",true},{"min",100},{"max",1000}};metadata["view"]=state;
    y=Studio::energyCoordinates(q,metadata,visible);assert(!visible[0] && !visible[3] && y[1]==0 && y[2]==1);
    // Object overrides supersede group style, which supersedes global style.
    state["level_style"]=QJsonObject{{"color","#0000ff"},{"line_width",2}};
    state["level_styles"]=QJsonObject{{"2",QJsonObject{{"color","#00ff00"},{"line_width",6}}}};metadata["view"]=state;
    assert(Studio::objectStyle(metadata,"band","1",false)["color"]=="#ff0000");
    assert(Studio::objectStyle(metadata,"band","2",false)["color"]=="#00ff00");
    assert(Studio::objectStyle(metadata,"band","a",true)["color"]=="#ff0000");
    assert(Studio::objectStyle(metadata,"","0",false)["color"]=="#0000ff");
    state.remove("energy");state.remove("focus");state["mode"]=1;metadata["view"]=state;
    QuiverView view;view.resize(1000,700);view.show();app.processEvents();view.setQuiver(&q);view.setMetadata(metadata);view.setMode(1);
    const auto levelLine=[&](const QColor& color)->QGraphicsLineItem* {for(auto* item:view.scene()->items())if(auto* line=dynamic_cast<QGraphicsLineItem*>(item))if(line->pen().color()==color && std::abs(line->line().dy())<.01)return line;return nullptr;};
    assert(levelLine(QColor("#00ff00")));assert(std::abs(levelLine(QColor("#00ff00"))->line().length()-180)<1e-9);
    // Drag the band heading: every member moves together and keeps its width.
    QGraphicsItem* heading=nullptr;for(auto* item:view.scene()->items())if(item->data(1).isValid())heading=item;assert(heading);
    auto from=view.mapFromScene(heading->sceneBoundingRect().center()),to=from+QPoint(80,0);double oldX=levelLine(QColor("#00ff00"))->line().center().x();double zoom=view.transform().m11();
    QMouseEvent down(QEvent::MouseButtonPress,from,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view.viewport(),&down);
    QMouseEvent move(QEvent::MouseMove,to,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view.viewport(),&move);
    QMouseEvent up(QEvent::MouseButtonRelease,to,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view.viewport(),&up);
    assert(std::abs(levelLine(QColor("#00ff00"))->line().center().x()-oldX-80/zoom)<2);
    auto capture=view.viewState();capture["highlight_level"]=2;capture["transition_offsets"]=QJsonObject{{"a",5.}};view.restoreView(capture);
    auto saved=view.viewState();assert(saved["highlight_level"]==2);assert(saved["transition_offsets"].toObject()["a"].toDouble()==5.);view.scale(1.7,1.7);view.centerOn(10,10);view.restoreView(saved);
    assert(std::abs(view.transform().m11()-saved["camera"].toObject()["zoom"].toDouble())<1e-9);
    auto center=view.mapToScene(view.viewport()->rect().center());assert(std::abs(center.x()-saved["camera"].toObject()["x"].toDouble())<3);
    // External arrows survive collapse; the two internal arrows disappear.
    auto layouts=state["group_layout"].toObject();auto band=layouts["band"].toObject();band["collapsed"]=true;layouts["band"]=band;state["group_layout"]=layouts;metadata["view"]=state;
    view.setMetadata(metadata);view.refresh();int summaries=0,aggregates=0;
    for(auto* item:view.scene()->items()){if(item->data(2)=="summary")++summaries;if(item->data(2)=="aggregate")++aggregates;}
    assert(summaries==1 && aggregates==2 && q.GetTransitions().size()==4);
    if(argc>1){view.fitInView(view.sceneRect(),Qt::KeepAspectRatio);assert(view.grab().save(argv[1]));}
    metadata["saved_views"]=QJsonArray{QJsonObject{{"name","Publication"},{"state",saved}}};
    auto root=QJsonDocument::fromJson(R"({"levels":["ground","middle","upper","external"],"transitions":[{"name":"a","source_index":2,"target_index":1,"probability":0.5},{"name":"parallel","source_index":2,"target_index":1,"probability":0.5},{"name":"b","source_index":1,"target_index":0,"probability":1},{"name":"in","source_index":3,"target_index":2,"probability":1}]})").object();root["metadata"]=metadata;root["level_energies_keV"]=QJsonArray{0,100,1000,2000};
    auto restored=Studio::readJson(QJsonDocument(root).toJson());assert(restored.metadata==metadata);
    const auto reject=[&](QJsonObject bad){bool threw=false;try{Studio::validateViewMetadata(q,bad);}catch(const std::exception&){threw=true;}assert(threw);};
    auto bad=metadata;bad["groups"]=42;reject(bad);bad=metadata;bad["view"]=42;reject(bad);
    bad=metadata;auto subsets=bad["subquivers"].toArray();auto subset=subsets[0].toObject();subset["levels"]=QJsonArray{2};subsets[0]=subset;bad["subquivers"]=subsets;reject(bad);
    bad=metadata;auto badState=state;badState["energy"]=QJsonObject{{"mode","physical"},{"window",true},{"min",100},{"max",10}};bad["view"]=badState;reject(bad);
    // Deletion remaps document definitions, current styles and saved styles.
    Studio::removeViewLevel(metadata,0);q.RemoveLevel(q.GetLevels()[0]);Studio::pruneViewTransitions(metadata,q);Studio::validateViewMetadata(q,metadata);
    assert(metadata["groups"].toArray()[0].toObject()["levels"].toArray()==QJsonArray({0,1}));
    assert(metadata["saved_views"].toArray()[0].toObject()["state"].toObject()["level_styles"].toObject().contains("1"));
    QJsonObject legacy{{"subquivers",QJsonArray{QJsonObject{{"name","Legacy band"},{"levels",QJsonArray{0,1}},{"color","#ff0000"}}}},{"focus_group",0}};
    auto migrated=Studio::normalizeViewMetadata(legacy);assert(migrated["groups"].toArray().size()==1 && migrated["subquivers"].toArray().empty());Studio::validateViewMetadata(q,migrated);
    if(argc>2) {
        QFile fixture(argv[2]);assert(fixture.open(QIODevice::ReadOnly));auto demo=Studio::readJson(fixture.readAll());
        assert(demo.metadata["saved_views"].toArray().size()==3);
        for(auto value:demo.metadata["saved_views"].toArray()) {
            auto m=demo.metadata;m["view"]=value.toObject()["state"];auto projection=Studio::selectGraph(*demo.quiver,m,m["view"].toObject()["mode"].toInt()==2);
            Studio::energyCoordinates(*demo.quiver,m,projection.levels);
        }
    }
    std::cout<<"PASS: explicit selections, context, energy transforms, styles, band dragging, collapse, saved views, migration and deletion\n";
}
