#include "MainWindow.h"
#include "QuiverView.h"
#include "ViewState.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QListWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QUuid>
#include <QColor>
#include <algorithm>

namespace {
QDoubleSpinBox* number(double minimum,double maximum,double value) {
    auto* box=new QDoubleSpinBox();box->setDecimals(3);box->setRange(minimum,maximum);box->setValue(value);return box;
}
void removeDefinition(QJsonObject& metadata,const QString& key,const QString& id) {
    auto items=metadata[key].toArray();
    for(int i=items.size()-1;i>=0;--i) if(items[i].toObject()["id"]==id) items.removeAt(i);
    metadata[key]=items;
    const auto clean=[&](QJsonObject state) {
        auto focus=state["focus"].toObject();
        if(focus["id"]==id && focus["type"]==(key=="groups"?"group":"subquiver")) state.remove("focus");
        if(key=="groups") {auto layouts=state["group_layout"].toObject();layouts.remove(id);state["group_layout"]=layouts;}
        return state;
    };
    metadata["view"]=clean(metadata["view"].toObject());auto views=metadata["saved_views"].toArray();
    for(int i=0;i<views.size();++i) {auto v=views[i].toObject();v["state"]=clean(v["state"].toObject());views[i]=v;}
    if(metadata.contains("saved_views"))metadata["saved_views"]=views;
}
}

void MainWindow::editGroups() {
    fMetadata=Studio::normalizeViewMetadata(fMetadata);
    QDialog dialog(this);dialog.setWindowTitle("Groups and band layout");dialog.resize(650,650);
    auto* form=new QFormLayout(&dialog);auto saved=fMetadata["groups"].toArray();
    auto* choice=new QComboBox();choice->addItem("New group");for(auto v:saved)choice->addItem(v.toObject()["name"].toString());form->addRow("Group",choice);
    auto* name=new QLineEdit();name->setObjectName("groupName");form->addRow("Name",name);
    auto* kind=new QComboBox();kind->addItems({"band","cascade","group"});form->addRow("Meaning",kind);
    auto* members=new QListWidget();members->setObjectName("groupLevels");members->setSelectionMode(QAbstractItemView::MultiSelection);
    for(auto* level:fQuiver->GetLevels())members->addItem(QString::fromStdString(level->GetName()));form->addRow("Levels",members);
    auto* color=new QLineEdit("#0f8b80");form->addRow("Group colour",color);
    auto* x=number(-1e6,1e6,250);form->addRow("Band centre (scene units)",x);
    auto* width=number(30,2000,160);width->setObjectName("groupWidth");form->addRow("Level width (scene units)",width);
    auto* stroke=number(0,20,0);stroke->setSpecialValueText("Inherit global");form->addRow("Line width",stroke);
    auto* dash=new QComboBox();dash->addItems({"inherit","solid","dash","dot"});form->addRow("Line style",dash);
    auto* hidden=new QCheckBox("Hidden in this view");form->addRow(hidden);auto* collapse=new QCheckBox("Collapsed in this view");form->addRow(collapse);
    auto* focus=new QCheckBox("Focus this group after saving");form->addRow(focus);
    const auto load=[&](int i) {
        members->clearSelection();QJsonObject group,layout;
        if(i) {group=saved[i-1].toObject();layout=fMetadata["view"].toObject()["group_layout"].toObject()[group["id"].toString()].toObject();}
        name->setText(group["name"].toString());kind->setCurrentText(group.value("kind").toString("band"));color->setText(layout.value("color").toString("#0f8b80"));
        x->setValue(layout.value("x").toDouble(180+saved.size()*220));width->setValue(layout.value("width").toDouble(160));
        stroke->setValue(layout["level_style"].toObject()["line_width"].toDouble());dash->setCurrentText(layout["level_style"].toObject().value("line_style").toString("inherit"));
        hidden->setChecked(layout["hidden"].toBool());collapse->setChecked(layout["collapsed"].toBool());
        for(auto v:group["levels"].toArray())members->item(v.toInt())->setSelected(true);
    };
    connect(choice,QOverload<int>::of(&QComboBox::currentIndexChanged),&dialog,load);load(0);
    auto* remove=new QPushButton("Delete group");form->addRow(remove);connect(remove,&QPushButton::clicked,&dialog,[&]{if(!choice->currentIndex())return;removeDefinition(fMetadata,"groups",saved[choice->currentIndex()-1].toObject()["id"].toString());updateUI();dialog.accept();});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{
        try {
            if(name->text().trimmed().isEmpty() || members->selectedItems().isEmpty())throw std::runtime_error("Name the group and select at least one level.");
            if(!QColor(color->text()).isValid())throw std::runtime_error("Use a valid colour name or #rrggbb.");
            auto candidate=fMetadata;int index=choice->currentIndex()-1;QString id=index<0?QUuid::createUuid().toString(QUuid::WithoutBraces):saved[index].toObject()["id"].toString();QJsonArray ids;
            for(auto* item:members->selectedItems())ids.append(members->row(item));
            QJsonObject group{{"id",id},{"name",name->text().trimmed()},{"kind",kind->currentText()},{"levels",ids}};
            auto definitions=saved;if(index<0)definitions.append(group);else definitions[index]=group;candidate["groups"]=definitions;
            auto state=fView->viewState();auto layouts=state["group_layout"].toObject();auto layout=layouts[id].toObject();
            layout["x"]=x->value();layout["width"]=width->value();layout["color"]=color->text();layout["hidden"]=hidden->isChecked();layout["collapsed"]=collapse->isChecked();
            QJsonObject style;if(stroke->value()>0)style["line_width"]=stroke->value();if(dash->currentIndex())style["line_style"]=dash->currentText();layout["level_style"]=style;layout["transition_style"]=style;layouts[id]=layout;state["group_layout"]=layouts;
            if(focus->isChecked()){state["focus"]=QJsonObject{{"type","group"},{"id",id}};state["mode"]=2;}
            candidate["view"]=state;Studio::validateViewMetadata(*fQuiver,candidate);fMetadata=candidate;updateUI();fView->configureView(state);fView->fitInView(fView->sceneRect(),Qt::KeepAspectRatio);dialog.accept();
        } catch(const std::exception& e){QMessageBox::warning(&dialog,"Group",e.what());}
    });dialog.exec();
}

void MainWindow::editSubquivers() {
    fMetadata=Studio::normalizeViewMetadata(fMetadata);
    QDialog dialog(this);dialog.setWindowTitle("Explicit subquivers");dialog.resize(720,650);auto* form=new QFormLayout(&dialog);
    auto saved=fMetadata["subquivers"].toArray();auto* choice=new QComboBox();choice->addItem("New subquiver");for(auto v:saved)choice->addItem(v.toObject()["name"].toString());form->addRow("Subquiver",choice);
    auto* name=new QLineEdit();name->setObjectName("subquiverName");form->addRow("Name",name);
    auto* levels=new QListWidget();levels->setObjectName("subquiverLevels");levels->setSelectionMode(QAbstractItemView::MultiSelection);for(auto* l:fQuiver->GetLevels())levels->addItem(QString::fromStdString(l->GetName()));form->addRow("Vertices",levels);
    auto* edges=new QListWidget();edges->setObjectName("subquiverTransitions");edges->setSelectionMode(QAbstractItemView::MultiSelection);for(auto* t:fQuiver->GetTransitions())edges->addItem(QString::fromStdString(t->GetName()));form->addRow("Arrows",edges);
    form->addRow(new QLabel("Selecting an arrow also includes its endpoints. Unselected arrows stay excluded."));
    auto* induced=new QPushButton("Select all arrows between selected levels");form->addRow(induced);
    connect(induced,&QPushButton::clicked,&dialog,[&]{const auto& vertices=fQuiver->GetLevels();for(std::size_t i=0;i<fQuiver->GetTransitions().size();++i){auto* t=fQuiver->GetTransitions()[i];int a=std::find(vertices.begin(),vertices.end(),t->GetSource())-vertices.begin(),b=std::find(vertices.begin(),vertices.end(),t->GetTarget())-vertices.begin();edges->item(i)->setSelected(levels->item(a)->isSelected() && levels->item(b)->isSelected());}});
    connect(edges,&QListWidget::itemSelectionChanged,&dialog,[&]{const auto& vertices=fQuiver->GetLevels();for(auto* item:edges->selectedItems()){auto* t=fQuiver->GetTransitions()[edges->row(item)];levels->item(std::find(vertices.begin(),vertices.end(),t->GetSource())-vertices.begin())->setSelected(true);levels->item(std::find(vertices.begin(),vertices.end(),t->GetTarget())-vertices.begin())->setSelected(true);}});
    connect(choice,QOverload<int>::of(&QComboBox::currentIndexChanged),&dialog,[&](int i){edges->clearSelection();levels->clearSelection();auto subset=i?saved[i-1].toObject():QJsonObject{};name->setText(subset["name"].toString());for(auto v:subset["levels"].toArray())levels->item(v.toInt())->setSelected(true);for(int j=0;j<edges->count();++j)edges->item(j)->setSelected(subset["transitions"].toArray().contains(edges->item(j)->text()));});
    auto* focus=new QCheckBox("Focus this subquiver after saving");focus->setChecked(true);form->addRow(focus);
    auto* remove=new QPushButton("Delete subquiver");form->addRow(remove);connect(remove,&QPushButton::clicked,&dialog,[&]{if(!choice->currentIndex())return;removeDefinition(fMetadata,"subquivers",saved[choice->currentIndex()-1].toObject()["id"].toString());updateUI();dialog.accept();});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{try{
        if(name->text().trimmed().isEmpty() || levels->selectedItems().isEmpty())throw std::runtime_error("Name the subquiver and select vertices/arrows.");
        QJsonArray vertices,arrows;for(auto* item:levels->selectedItems())vertices.append(levels->row(item));for(auto* item:edges->selectedItems())arrows.append(item->text());
        int index=choice->currentIndex()-1;QString id=index<0?QUuid::createUuid().toString(QUuid::WithoutBraces):saved[index].toObject()["id"].toString();
        QJsonObject subset{{"id",id},{"name",name->text().trimmed()},{"levels",vertices},{"transitions",arrows}};auto definitions=saved;if(index<0)definitions.append(subset);else definitions[index]=subset;
        auto candidate=fMetadata;candidate["subquivers"]=definitions;auto state=fView->viewState();if(focus->isChecked()){state["focus"]=QJsonObject{{"type","subquiver"},{"id",id}};state["mode"]=2;}candidate["view"]=state;
        Studio::validateViewMetadata(*fQuiver,candidate);fMetadata=candidate;updateUI();fView->configureView(state);fView->fitInView(fView->sceneRect(),Qt::KeepAspectRatio);dialog.accept();
    }catch(const std::exception& e){QMessageBox::warning(&dialog,"Subquiver",e.what());}});dialog.exec();
}

void MainWindow::editViewSettings() {
    QDialog dialog(this);dialog.setWindowTitle("Focus, energy and global style");auto* form=new QFormLayout(&dialog);auto state=fView->viewState();
    auto* target=new QComboBox();target->setObjectName("focusTarget");target->addItem("Full quiver",QJsonObject{});
    for(const QString key:{"groups","subquivers"})for(auto v:fMetadata[key].toArray()){auto item=v.toObject();target->addItem((key=="groups"?"Group: ":"Subquiver: ")+item["name"].toString(),QJsonObject{{"type",key=="groups"?"group":"subquiver"},{"id",item["id"]}});}
    for(std::size_t i=0;i<fQuiver->GetLevels().size();++i)target->addItem("Level: "+QString::fromStdString(fQuiver->GetLevels()[i]->GetName()),QJsonObject{{"type","level"},{"level",int(i)}});
    auto selected=state["focus"].toObject();for(int i=0;i<target->count();++i){auto data=target->itemData(i).toJsonObject();if(data["type"]==selected["type"] && data["id"]==selected["id"] && data["level"]==selected["level"])target->setCurrentIndex(i);}form->addRow("Focus",target);
    auto* context=new QComboBox();context->addItems({"none","incoming","outgoing","both","ancestors","descendants"});context->setCurrentText(selected.value("context").toString("none"));form->addRow("Include context",context);
    auto* depth=new QSpinBox();depth->setRange(1,100);depth->setValue(selected.value("depth").toInt(1));form->addRow("Neighbour depth",depth);
    auto energy=state["energy"].toObject();auto* mode=new QComboBox();mode->setObjectName("energyMode");mode->addItems({"auto","physical","compressed","uniform"});mode->setCurrentText(energy.value("mode").toString("auto"));form->addRow("Energy spacing",mode);
    auto* window=new QCheckBox("Limit energy range (keV)");window->setChecked(energy["window"].toBool());form->addRow(window);
    auto* min=number(0,1e9,energy.value("min").toDouble(0));auto* max=number(0,1e9,energy.value("max").toDouble(3000));form->addRow("Minimum keV",min);form->addRow("Maximum keV",max);
    auto* compression=number(.001,1e9,energy.value("compression_keV").toDouble(100));form->addRow("Compression E₀ (keV)",compression);
    auto* levelColor=new QLineEdit(state["level_style"].toObject().value("color").toString("#0f8b80"));form->addRow("Global level colour",levelColor);
    auto* edgeColor=new QLineEdit(state["transition_style"].toObject().value("color").toString("#647b91"));form->addRow("Global transition colour",edgeColor);
    auto* labels=new QCheckBox("Show transition labels by default");labels->setChecked(state["transition_style"].toObject().value("labels").toBool(true));form->addRow(labels);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Close);form->addRow(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,&dialog,[&]{try{
        auto next=state;auto f=target->currentData().toJsonObject();if(f.isEmpty())next.remove("focus");else{f["context"]=context->currentText();f["depth"]=depth->value();next["focus"]=f;}
        next["mode"]=f.isEmpty()?(fView->mode()==2?0:fView->mode()):2;
        next["energy"]=QJsonObject{{"mode",mode->currentText()},{"window",window->isChecked()},{"min",min->value()},{"max",max->value()},{"compression_keV",compression->value()}};
        auto ls=next["level_style"].toObject();ls["color"]=levelColor->text();next["level_style"]=ls;auto ts=next["transition_style"].toObject();ts["color"]=edgeColor->text();ts["labels"]=labels->isChecked();next["transition_style"]=ts;
        auto candidate=fMetadata;candidate["view"]=next;Studio::validateViewMetadata(*fQuiver,candidate);auto selection=Studio::selectGraph(*fQuiver,candidate,next["mode"].toInt()==2);Studio::energyCoordinates(*fQuiver,candidate,selection.levels);
        fView->configureView(next);fView->fitInView(fView->sceneRect(),Qt::KeepAspectRatio);state=next;
    }catch(const std::exception& e){QMessageBox::warning(&dialog,"View settings",e.what());}});dialog.exec();
}

void MainWindow::manageViews() {
    QDialog dialog(this);dialog.setWindowTitle("Saved views");dialog.resize(500,400);auto* layout=new QVBoxLayout(&dialog);
    auto* list=new QListWidget();list->setObjectName("savedViews");layout->addWidget(list);auto* name=new QLineEdit();name->setPlaceholderText("View name");name->setObjectName("viewName");layout->addWidget(name);
    const auto reload=[&]{list->clear();for(auto v:fMetadata["saved_views"].toArray())list->addItem(v.toObject()["name"].toString());};reload();
    connect(list,&QListWidget::currentTextChanged,name,&QLineEdit::setText);
    auto* save=new QPushButton("Save current view");auto* load=new QPushButton("Restore selected view");auto* remove=new QPushButton("Delete selected view");layout->addWidget(save);layout->addWidget(load);layout->addWidget(remove);
    connect(save,&QPushButton::clicked,&dialog,[&]{try{
        auto label=name->text().trimmed();if(label.isEmpty())throw std::runtime_error("Name the view.");
        auto state=fView->viewState();auto saved=fMetadata["saved_views"].toArray();QJsonObject entry{{"name",label},{"state",state}};int found=-1;
        for(int i=0;i<saved.size();++i)if(saved[i].toObject()["name"]==label)found=i;
        if(found<0)saved.append(entry);else saved[found]=entry;auto candidate=fMetadata;candidate["saved_views"]=saved;candidate["view"]=state;Studio::validateViewMetadata(*fQuiver,candidate);fMetadata=candidate;fView->setMetadata(fMetadata);reload();
    }catch(const std::exception& e){QMessageBox::warning(&dialog,"Save view",e.what());}});
    connect(load,&QPushButton::clicked,&dialog,[&]{int i=list->currentRow();if(i<0)return;auto state=fMetadata["saved_views"].toArray()[i].toObject()["state"].toObject();fView->restoreView(state);});
    connect(remove,&QPushButton::clicked,&dialog,[&]{int i=list->currentRow();if(i<0)return;auto saved=fMetadata["saved_views"].toArray();saved.removeAt(i);fMetadata["saved_views"]=saved;fView->setMetadata(fMetadata);reload();});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();
}
