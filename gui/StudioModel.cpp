#include "StudioModel.h"
#include "ViewState.h"
#include <QJsonArray>
#include <QRegularExpression>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
}
DecayVector Studio::transitionVector(const DecayQuiver& q) {
    DecayVector v;
    for (auto* t:q.GetTransitions()) v.AddTerm(DecayPath(std::vector<DecayTransition>{*t}), t->GetProbability());
    return v;
}
DecayVector Studio::branchingVector(const DecayQuiver& q, const QJsonObject& m) {
    if (!m.contains("branching")) {
        auto decay = createDecayVector(q,m);
        DecayVector v;
        for (const auto& term:decay.GetTerms()) if (term.path.IsStationary()) v.AddTerm(term.path,term.coefficient);
        return v;
    }
    require(m["branching"].isArray(), "Branching must be an array.");
    auto a=m["branching"].toArray();
    require(a.size()==int(q.GetLevels().size()), "Branching must contain one fraction per level.");
    DecayVector v; double sum=0;
    for(int i=0;i<a.size();++i) {
        double x=a[i].toDouble(-1);
        require(a[i].isDouble() && std::isfinite(x) && x>=0 && x<=1,"Branching fractions must lie in [0,1].");
        sum+=x; if(x>0) v.AddTerm(DecayPath(q.GetLevels()[i]),x);
    }
    require(std::abs(sum-1)<1e-8,"Initial level populations must sum to 1.");
    return v;
}
double Studio::levelEnergy(const DecayQuiver& q,const QJsonObject& m,int i) {
    Q_UNUSED(m);
    const auto* level=q.GetLevels().at(i);
    require(level->HasEnergy(),"Set level energies in keV before using energy scale or efficiencies.");
    return level->GetEnergy();
}
QJsonArray Studio::readEfficiencyCsv(const QByteArray& bytes) {
    QJsonArray result;
    auto text=QString::fromUtf8(bytes);
    if(text.startsWith(QChar(0xfeff))) text.remove(0,1);
    auto lines=text.split('\n');
    bool first=true; double previous=-1;
    for(auto line:lines) {
        line=line.trimmed(); if(line.isEmpty() || line.startsWith('#')) continue;
        auto fields=line.split(',');
        require(fields.size()==2,"CSV requires two columns: energy_keV,efficiency or Energy[keV],HPGe.");
        for(auto& field:fields) {
            field=field.trimmed();
            if(field.startsWith('"') && field.endsWith('"')) field=field.mid(1,field.size()-2).trimmed();
        }
        const auto energyHeader=fields[0].toLower();
        const auto efficiencyHeader=fields[1].toLower();
        if(first && (energyHeader=="energy_kev" || energyHeader=="energy[kev]") &&
           (efficiencyHeader=="efficiency" || efficiencyHeader=="hpge")) {
            first=false; continue;
        }
        first=false;
        bool a,b; double e=fields[0].trimmed().toDouble(&a),p=fields[1].trimmed().toDouble(&b);
        require(a && b && std::isfinite(e) && std::isfinite(p) && e>=0 && e>previous && p>=0 && p<=1,
                "CSV energies must increase strictly; efficiencies must be fractions in [0,1].");
        result.append(QJsonArray{e,p}); previous=e;
    }
    require(result.size()>=2,"Supply at least two efficiency samples."); return result;
}
std::unordered_map<std::string,double> Studio::efficiencyMap(const DecayQuiver& q,const QJsonObject& m) {
    auto samples=m["efficiency_samples"].toArray();
    require(samples.size()>=2,"Import an efficiency CSV first.");
    double last=-1;
    for(auto sample:samples) {
        auto a=sample.toArray(); double e=a.size()==2?a[0].toDouble(-1):-1, p=a.size()==2?a[1].toDouble(-1):-1;
        require(a.size()==2 && a[0].isDouble() && a[1].isDouble() && std::isfinite(e) && std::isfinite(p) && e>=0 && e>last && p>=0 && p<=1,"Invalid efficiency samples."); last=e;
    }
    std::unordered_map<std::string,double> map;
    const auto& levels=q.GetLevels();
    for(auto* t:q.GetTransitions()) {
        int s=std::find(levels.begin(),levels.end(),t->GetSource())-levels.begin();
        int d=std::find(levels.begin(),levels.end(),t->GetTarget())-levels.begin();
        double e=std::abs(levelEnergy(q,m,s)-levelEnergy(q,m,d));
        if(e<samples.first().toArray()[0].toDouble() || e>last)
            throw std::runtime_error(QString("Transition %1 has energy %2 keV, outside the efficiency table (%3–%4 keV). Extrapolation is disabled.")
                .arg(QString::fromStdString(t->GetName())).arg(e,0,'g',12)
                .arg(samples.first().toArray()[0].toDouble()).arg(last).toStdString());
        for(int i=1;i<samples.size();++i) {
            auto a=samples[i-1].toArray(),b=samples[i].toArray();
            if(e<=b[0].toDouble()) { double f=(e-a[0].toDouble())/(b[0].toDouble()-a[0].toDouble()); map[t->GetName()]=a[1].toDouble()+f*(b[1].toDouble()-a[1].toDouble()); break; }
        }
    }
    return map;
}
std::unordered_map<std::string,double> Studio::conversionMap(const DecayQuiver& q,const QJsonObject& m) {
    std::unordered_map<std::string,double> map;
    if(!m.contains("conversion_coefficients")) {
        for(auto* t:q.GetTransitions()) map[t->GetName()]=0.0;
        return map;
    }
    require(m["conversion_coefficients"].isObject(),"IC coefficients must be an object keyed by transition name.");
    require(m.value("transition_probability_basis").toString("gamma_plus_ic")=="gamma_plus_ic",
            "IC correction requires total gamma+IC transition probabilities.");
    const auto coefficients=m["conversion_coefficients"].toObject();
    require(coefficients.size()==int(q.GetTransitions().size()),"IC coefficients must cover every transition exactly.");
    for(auto* t:q.GetTransitions()) {
        auto value=coefficients[QString::fromStdString(t->GetName())];
        if(!value.isDouble() || !std::isfinite(value.toDouble()) || value.toDouble()<0)
            throw std::runtime_error("Missing or invalid IC coefficient for "+t->GetName());
        map[t->GetName()]=value.toDouble();
    }
    return map;
}

void Studio::validateStudio(const DecayQuiver& q,const QJsonObject& m) {
    validateViewMetadata(q,m);
    conversionMap(q,m);
    if(m.contains("branching")) branchingVector(q,m);
    if(m.contains("efficiency_samples")) efficiencyMap(q,m);
    require(!m.contains("subquivers") || m["subquivers"].isArray(),"Subquivers must be an array.");
    require(!m.contains("level_energies_keV") || m["level_energies_keV"].isObject(),"Level energies must be an object.");
    auto energies=m["level_energies_keV"].toObject();
    for(auto it=energies.begin();it!=energies.end();++it) {
        bool ok; int i=it.key().toInt(&ok);
        require(ok && i>=0 && i<int(q.GetLevels().size()) && it.value().isDouble() && std::isfinite(it.value().toDouble()) && it.value().toDouble()>=0,"Invalid level energy entry.");
    }
    for(auto value:m["subquivers"].toArray()) {
        auto group=value.toObject();
        require(!group["name"].toString().trimmed().isEmpty() && group["levels"].isArray(),"Invalid subquiver.");
        for(auto id:group["levels"].toArray()) require(id.isDouble() && id.toDouble()==id.toInt(-1) && id.toInt(-1)>=0 && id.toInt()<(int)q.GetLevels().size(),"Unknown subquiver level.");
    }
}
