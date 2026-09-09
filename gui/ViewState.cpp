#include "ViewState.h"
#include "StudioModel.h"
#include <QColor>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace {
void require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
void merge(QJsonObject& into, const QJsonObject& from) {
    for (auto it=from.begin(); it!=from.end(); ++it) into[it.key()]=it.value();
}
void validateStyle(const QJsonObject& style) {
    if (style.contains("color")) require(QColor(style["color"].toString()).isValid(),"Invalid style colour.");
    if (style.contains("line_width")) require(style["line_width"].isDouble() && style["line_width"].toDouble()>0 && style["line_width"].toDouble()<=20,"Line width must be in (0,20].");
    if (style.contains("line_style")) require(QStringList{"solid","dash","dot"}.contains(style["line_style"].toString()),"Unknown line style.");
    if (style.contains("labels")) require(style["labels"].isBool(),"Label visibility must be boolean.");
}
}

QJsonObject Studio::normalizeViewMetadata(QJsonObject m) {
    // Earlier Studio documents stored semantic groups under 'subquivers'.
    if (!m.contains("groups") && m.contains("subquivers")) {
        QJsonArray groups, subsets;
        int i=0;
        for (auto v:m.value("subquivers").toArray()) {
            auto g=v.toObject();
            if (g.contains("transitions")) { subsets.append(g); continue; }
            g["id"]=g.value("id").toString(QString("group-%1").arg(i++)); groups.append(g);
        }
        m["groups"]=groups; m["subquivers"]=subsets;
    }
    if (!m.contains("groups") && !m.contains("view") && !m.contains("saved_views") && !m.contains("level_colors") && !m.contains("transition_colors")) return m;
    auto view=m.value("view").toObject(); auto layouts=view["group_layout"].toObject();
    auto groups=m.value("groups").toArray();
    for(int i=0;i<groups.size();++i) {
        auto g=groups[i].toObject(); auto id=g.value("id").toString(QString("group-%1").arg(i)); g["id"]=id;
        auto layout=layouts[id].toObject();
        for(const QString key:{"color","hidden","collapsed","x","width"})
            if(g.contains(key)) { layout[key]=g[key]; g.remove(key); }
        layouts[id]=layout; groups[i]=g;
    }
    if(m.contains("groups")) m["groups"]=groups;
    if(!layouts.isEmpty()) view["group_layout"]=layouts;
    if(m.contains("focus_group")) {
        int index=m.value("focus_group").toInt();
        if(index>=0 && index<groups.size()) view["focus"]=QJsonObject{{"type","group"},{"id",groups[index].toObject()["id"]}};
        m.remove("focus_group");
    }
    for(const QString key:{"level_colors","transition_colors"}) if(m.contains(key)) {
        const QString dest=key=="level_colors"?"level_styles":"transition_styles";
        auto styles=view[dest].toObject(); auto colors=m.value(key).toObject();
        for(auto it=colors.begin();it!=colors.end();++it) { auto style=styles[it.key()].toObject(); style["color"]=it.value(); styles[it.key()]=style; }
        view[dest]=styles; m.remove(key);
    }
    m["view"]=view;
    return m;
}

QJsonArray Studio::layoutGroups(const QJsonObject& metadata) {
    auto m=normalizeViewMetadata(metadata); auto groups=m.value("groups").toArray();
    auto layouts=m.value("view").toObject()["group_layout"].toObject();
    for(int i=0;i<groups.size();++i) { auto g=groups[i].toObject(); merge(g,layouts[g["id"].toString()].toObject()); groups[i]=g; }
    return groups;
}

Studio::GraphSelection Studio::selectGraph(const DecayQuiver& q,const QJsonObject& raw,bool focus) {
    auto m=normalizeViewMetadata(raw); auto view=m.value("view").toObject(); auto f=view["focus"].toObject();
    const auto& levels=q.GetLevels(); const auto& transitions=q.GetTransitions();
    GraphSelection result{std::vector<bool>(levels.size(),!focus),std::vector<bool>(transitions.size(),!focus),
                          std::vector<bool>(levels.size()),std::vector<bool>(transitions.size())};
    if(!focus) return result;
    bool explicitEdges=false;
    QJsonObject selected;
    const QString type=f["type"].toString();
    if(type=="group" || type=="subquiver") {
        for(auto v:m.value(type=="group"?"groups":"subquivers").toArray()) if(v.toObject()["id"]==f["id"]) selected=v.toObject();
        for(auto id:selected["levels"].toArray()) if(id.toInt()>=0 && id.toInt()<int(levels.size())) result.levels[id.toInt()]=true;
        explicitEdges=type=="subquiver";
    } else if(type=="level") {
        int index=f["level"].toInt(-1);
        if(index>=0 && index<int(levels.size())) result.levels[index]=true;
    } else { std::fill(result.levels.begin(),result.levels.end(),true); std::fill(result.transitions.begin(),result.transitions.end(),true); return result; }
    const auto endpoint=[&](const DecayLevel* l) { return std::find(levels.begin(),levels.end(),l)-levels.begin(); };
    auto base=result.levels;
    QString context=f.value("context").toString("none");
    int depth=f.value("depth").toInt(1);
    if(context=="ancestors" || context=="descendants") depth=levels.size();
    for(int step=0;step<depth && context!="none";++step) {
        auto before=result.levels;
        for(auto* t:transitions) {
            auto a=endpoint(t->GetSource()), b=endpoint(t->GetTarget());
            if(context=="incoming" || context=="ancestors" || context=="both") if(before[b]) result.levels[a]=true;
            if(context=="outgoing" || context=="descendants" || context=="both") if(before[a]) result.levels[b]=true;
        }
        if(before==result.levels) break;
    }
    auto names=selected["transitions"].toArray();
    for(std::size_t i=0;i<transitions.size();++i) {
        auto a=endpoint(transitions[i]->GetSource()), b=endpoint(transitions[i]->GetTarget());
        bool internal=base[a] && base[b];
        bool member=!explicitEdges || names.contains(QString::fromStdString(transitions[i]->GetName()));
        result.transitions[i]=result.levels[a] && result.levels[b] && ((!internal && context!="none") || (internal && member));
        result.contextTransitions[i]=result.transitions[i] && !internal;
    }
    for(std::size_t i=0;i<levels.size();++i) result.contextLevels[i]=result.levels[i] && !base[i];
    return result;
}

QJsonObject Studio::objectStyle(const QJsonObject& m,const QString& group,const QString& id,bool transition) {
    auto view=m.value("view").toObject();
    QJsonObject style{{"color",transition?"#647b91":"#0f8b80"},{"line_width",transition?3.:4.},{"line_style","solid"},{"labels",true}};
    merge(style,view[transition?"transition_style":"level_style"].toObject());
    auto layout=view["group_layout"].toObject()[group].toObject();
    if(layout.contains("color")) style["color"]=layout["color"];
    merge(style,layout[transition?"transition_style":"level_style"].toObject());
    merge(style,view[transition?"transition_styles":"level_styles"].toObject()[id].toObject());
    return style;
}

std::vector<double> Studio::energyCoordinates(const DecayQuiver& q,const QJsonObject& m,std::vector<bool>& visible) {
    auto settings=m.value("view").toObject()["energy"].toObject();
    QString mode=settings.value("mode").toString("auto");
    std::vector<double> e(visible.size()), result(visible.size()); bool known=true;
    for(std::size_t i=0;i<visible.size();++i) if(visible[i]) {
        try { e[i]=levelEnergy(q,m,i); } catch(const std::exception&) { known=false; e[i]=i; }
    }
    require(known || (mode=="auto" || mode=="uniform") && !settings.value("window").toBool(),"Set energies for all selected levels before using this energy scale/window.");
    if(settings.value("window").toBool()) for(std::size_t i=0;i<visible.size();++i)
        visible[i]=visible[i] && e[i]>=settings["min"].toDouble() && e[i]<=settings["max"].toDouble();
    std::vector<int> order;
    for(std::size_t i=0;i<visible.size();++i) if(visible[i]) order.push_back(i);
    if(order.empty()) return result;
    if(mode=="uniform" || !known) {
        if(known) std::stable_sort(order.begin(),order.end(),[&](int a,int b){return e[a]<e[b];});
        for(std::size_t rank=0;rank<order.size();++rank) result[order[rank]]=double(rank)/std::max(std::size_t(1),order.size()-1);
    } else {
        double lo=e[order.front()],hi=lo;
        for(int i:order) {lo=std::min(lo,e[i]);hi=std::max(hi,e[i]);}
        if(settings.value("window").toBool()) {lo=settings["min"].toDouble();hi=settings["max"].toDouble();}
        double e0=settings.value("compression_keV").toDouble(100.);
        const auto transform=[&](double value){return mode=="compressed"?std::log1p(value/e0):value;};
        double span=transform(hi)-transform(lo);
        for(int i:order) result[i]=span>0?(transform(e[i])-transform(lo))/span:0;
    }
    return result;
}

void Studio::validateViewMetadata(const DecayQuiver& q,const QJsonObject& raw) {
    for(const QString key:{"groups","subquivers","saved_views"}) require(!raw.contains(key) || raw.value(key).isArray(),"Group, subquiver and saved-view collections must be arrays.");
    require(!raw.contains("view") || raw.value("view").isObject(),"View must be an object.");
    for(const QString key:{"groups","subquivers"}) for(auto v:raw.value(key).toArray()) require(v.isObject(),"Group/subquiver entries must be objects.");
    auto m=normalizeViewMetadata(raw); QSet<QString> groupIds,subsetIds;
    for(const QString key:{"groups","subquivers"}) {
        require(!m.contains(key) || m.value(key).isArray(),"Groups and subquivers must be arrays.");
        auto& ids=key=="groups"?groupIds:subsetIds;
        for(auto value:m.value(key).toArray()) {
            auto item=value.toObject(); QString id=item["id"].toString();
            require(!id.isEmpty() && !ids.contains(id) && !item["name"].toString().trimmed().isEmpty(),"Each group/subquiver needs a unique ID and a name."); ids.insert(id);
            require(item["levels"].isArray(),"Graph membership requires a levels array."); QSet<int> members;
            for(auto v:item["levels"].toArray()) {
                int i=v.toInt(-1); require(v.isDouble() && v.toDouble()==i && i>=0 && i<int(q.GetLevels().size()) && !members.contains(i),"Invalid or duplicate member level."); members.insert(i);
            }
            if(key=="subquivers") {
                require(item["transitions"].isArray(),"Explicit subquivers require a transitions array."); QSet<QString> edges;
                for(auto v:item["transitions"].toArray()) {
                    QString name=v.toString(); auto* t=q.GetTransition(name.toStdString());
                    require(v.isString() && t && !edges.contains(name),"Invalid or duplicate member transition."); edges.insert(name);
                    const auto& levels=q.GetLevels();
                    require(members.contains(std::find(levels.begin(),levels.end(),t->GetSource())-levels.begin()) && members.contains(std::find(levels.begin(),levels.end(),t->GetTarget())-levels.begin()),"Subquiver arrows require both endpoint levels.");
                }
            }
        }
    }
    const auto validateView=[&](const QJsonObject& view) {
        for(const QString key:{"focus","energy","camera","group_layout","level_style","transition_style","level_styles","transition_styles","level_y","transition_offsets"}) require(!view.contains(key) || view[key].isObject(),"View settings must be objects.");
        if(view.contains("mode")) require(view["mode"].isDouble() && view["mode"].toDouble()==view["mode"].toInt() && view["mode"].toInt()>=0 && view["mode"].toInt()<=2,"Invalid view mode.");
        auto focus=view["focus"].toObject(); auto type=focus["type"].toString();
        require(type.isEmpty() || type=="group" || type=="subquiver" || type=="level","Unknown focus type.");
        if(type=="group") require(groupIds.contains(focus["id"].toString()),"Focus references a missing group.");
        if(type=="subquiver") require(subsetIds.contains(focus["id"].toString()),"Focus references a missing subquiver.");
        if(type=="level") require(focus["level"].isDouble() && focus["level"].toDouble()==focus["level"].toInt(-1) && focus["level"].toInt(-1)>=0 && focus["level"].toInt()<int(q.GetLevels().size()),"Invalid focus level.");
        require(QStringList{"none","incoming","outgoing","both","ancestors","descendants"}.contains(focus.value("context").toString("none")),"Unknown focus context.");
        if(focus.contains("depth")) require(focus["depth"].isDouble() && focus["depth"].toDouble()==focus["depth"].toInt() && focus["depth"].toInt()>=1 && focus["depth"].toInt()<=100,"Focus depth must be an integer from 1 to 100.");
        auto energy=view["energy"].toObject();
        require(QStringList{"auto","physical","compressed","uniform"}.contains(energy.value("mode").toString("auto")),"Unknown energy scale.");
        if(energy.contains("window")) require(energy["window"].isBool(),"Energy window flag must be boolean.");
        if(energy.value("window").toBool()) require(energy["min"].isDouble() && energy["max"].isDouble() && energy["min"].toDouble()>=0 && energy["max"].toDouble()>energy["min"].toDouble(),"Invalid energy window.");
        if(energy.contains("compression_keV")) require(energy["compression_keV"].isDouble() && energy["compression_keV"].toDouble()>0,"Compression energy must be positive.");
        if(view.contains("energy_scale")) require(view["energy_scale"].isDouble() && view["energy_scale"].toDouble()>=.25 && view["energy_scale"].toDouble()<=10,"Invalid vertical scale.");
        auto layouts=view["group_layout"].toObject();
        for(auto it=layouts.begin();it!=layouts.end();++it) {
            require(groupIds.contains(it.key()) && it.value().isObject(),"Layout references a missing group."); auto l=it.value().toObject();
            if(l.contains("x")) require(l["x"].isDouble() && std::abs(l["x"].toDouble())<=1e6,"Invalid band position.");
            if(l.contains("width")) require(l["width"].isDouble() && l["width"].toDouble()>=30 && l["width"].toDouble()<=2000,"Band width must be 30–2000 scene units.");
            for(const QString key:{"hidden","collapsed"}) if(l.contains(key)) require(l[key].isBool(),"Group visibility and collapse flags must be boolean.");
            for(const QString key:{"level_style","transition_style"}) if(l.contains(key)) require(l[key].isObject(),"Group styles must be objects.");
            validateStyle(l); validateStyle(l["level_style"].toObject()); validateStyle(l["transition_style"].toObject());
        }
        validateStyle(view["level_style"].toObject()); validateStyle(view["transition_style"].toObject());
        for(const QString key:{"level_styles","transition_styles"}) { auto styles=view[key].toObject(); for(auto it=styles.begin();it!=styles.end();++it) {
            if(key=="level_styles") {bool ok; int i=it.key().toInt(&ok); require(ok && i>=0 && i<int(q.GetLevels().size()),"Style references a missing level.");}
            else require(q.GetTransition(it.key().toStdString())!=nullptr,"Style references a missing transition.");
            require(it.value().isObject(),"Invalid object style."); validateStyle(it.value().toObject());
        }}
        for(const QString key:{"level_y","transition_offsets"}) {auto values=view[key].toObject();for(auto it=values.begin();it!=values.end();++it) {
            require(it.value().isDouble() && std::abs(it.value().toDouble())<=1e6,"Invalid saved arrangement coordinate.");
            if(key=="level_y"){bool ok;int i=it.key().toInt(&ok);require(ok && i>=0 && i<int(q.GetLevels().size()),"Saved arrangement references a missing level.");}
            else require(q.GetTransition(it.key().toStdString())!=nullptr,"Saved arrangement references a missing transition.");
        }}
        if(view.contains("highlight_level")){int i=view["highlight_level"].toInt(-1);require(view["highlight_level"].isDouble() && view["highlight_level"].toDouble()==i && i>=0 && i<int(q.GetLevels().size()),"Invalid highlighted level.");}
        if(view.contains("highlight_transition"))require(view["highlight_transition"].isString() && q.GetTransition(view["highlight_transition"].toString().toStdString()),"Invalid highlighted transition.");
        auto camera=view["camera"].toObject();
        if(!camera.isEmpty()) require(camera["zoom"].isDouble() && camera["zoom"].toDouble()>=.01 && camera["zoom"].toDouble()<=100 && camera["x"].isDouble() && camera["y"].isDouble() && std::abs(camera["x"].toDouble())<=1e7 && std::abs(camera["y"].toDouble())<=1e7,"Invalid saved camera.");
    };
    require(!m.contains("view") || m.value("view").isObject(),"View must be an object."); validateView(m.value("view").toObject());
    require(!m.contains("saved_views") || m.value("saved_views").isArray(),"Saved views must be an array."); QSet<QString> names;
    for(auto v:m.value("saved_views").toArray()) {auto saved=v.toObject(); QString name=saved["name"].toString(); require(!name.trimmed().isEmpty() && !names.contains(name) && saved["state"].isObject(),"Saved views require unique names and state objects."); names.insert(name); validateView(saved["state"].toObject());}
}

void Studio::removeViewLevel(QJsonObject& m,int index) {
    m=normalizeViewMetadata(m);
    for(const QString key:{"groups","subquivers"}) if(m.contains(key)) {
        auto items=m.value(key).toArray();
        for(int j=0;j<items.size();++j) {auto item=items[j].toObject(); QJsonArray members;
            for(auto v:item["levels"].toArray()) {int i=v.toInt();if(i!=index) members.append(i>index?i-1:i);} item["levels"]=members;items[j]=item;}
        m[key]=items;
    }
    const auto remap=[&](QJsonObject state) {
        auto focus=state.value("focus").toObject(); if(focus["type"]=="level") {int i=focus["level"].toInt();if(i==index) state.remove("focus");else {focus["level"]=i>index?i-1:i;state["focus"]=focus;}}
        if(state.contains("highlight_level")){int i=state.value("highlight_level").toInt();if(i==index)state.remove("highlight_level");else state["highlight_level"]=i>index?i-1:i;}
        for(const QString key:{"level_styles","level_y"}) {
            auto old=state.value(key).toObject();QJsonObject next;
            for(auto it=old.begin();it!=old.end();++it){int i=it.key().toInt();if(i!=index)next[QString::number(i>index?i-1:i)]=it.value();}
            if(state.contains(key))state[key]=next;
        }
        return state;
    };
    if(m.contains("view")) m["view"]=remap(m.value("view").toObject());
    auto saved=m.value("saved_views").toArray(); for(int i=0;i<saved.size();++i) {auto v=saved[i].toObject();v["state"]=remap(v["state"].toObject());saved[i]=v;} if(m.contains("saved_views")) m["saved_views"]=saved;
}

void Studio::pruneViewTransitions(QJsonObject& m,const DecayQuiver& q) {
    auto subsets=m.value("subquivers").toArray();
    for(int i=0;i<subsets.size();++i) {auto subset=subsets[i].toObject();auto members=subset["levels"].toArray();QJsonArray names;
        for(auto v:subset["transitions"].toArray()) if(auto* t=q.GetTransition(v.toString().toStdString())) {
            auto& levels=q.GetLevels(); int a=std::find(levels.begin(),levels.end(),t->GetSource())-levels.begin(),b=std::find(levels.begin(),levels.end(),t->GetTarget())-levels.begin();
            if(members.contains(a) && members.contains(b)) names.append(v);
        } subset["transitions"]=names;subsets[i]=subset;
    } if(m.contains("subquivers")) m["subquivers"]=subsets;
    const auto prune=[&](QJsonObject state) {
        for(const QString key:{"transition_styles","transition_offsets"}) {auto values=state.value(key).toObject();for(auto it=values.begin();it!=values.end();) {if(!q.GetTransition(it.key().toStdString()))it=values.erase(it);else ++it;}if(state.contains(key))state[key]=values;}
        if(state.contains("highlight_transition") && !q.GetTransition(state.value("highlight_transition").toString().toStdString()))state.remove("highlight_transition");
        return state;
    };
    if(m.contains("view"))m["view"]=prune(m.value("view").toObject());auto saved=m.value("saved_views").toArray();for(int i=0;i<saved.size();++i){auto v=saved[i].toObject();v["state"]=prune(v["state"].toObject());saved[i]=v;}if(m.contains("saved_views"))m["saved_views"]=saved;
}
