#include "COINAlgebra/Core/DecayQuiver.h"

#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <set>
#include <sstream>
#include <iostream>
#include <stdexcept>


DecayQuiver::DecayQuiver()
{
}


DecayQuiver::~DecayQuiver()
{
    for(auto* transition : fTransitions)
    {
        delete transition;
    }

    for(auto* level : fLevels)
    {
        delete level;
    }
}


// ============================================================
// Vertices
// ============================================================

DecayLevel* DecayQuiver::AddLevel(
    const std::string& name
)
{
    if(GetLevel(name) != nullptr)
    {
        throw std::invalid_argument(
            "Decay level '" + name + "' already exists."
        );
    }

    auto* level = new DecayLevel(name);

    fLevels.push_back(level);

    return level;
}


DecayLevel* DecayQuiver::AddLevel(const std::string& name, double energy_keV)
{
    // Validate before adding a vertex, so a failed call leaves the graph intact.
    DecayLevel checked(name, energy_keV);
    auto* level = AddLevel(name);
    level->SetEnergy(checked.GetEnergy());
    return level;
}


DecayLevel* DecayQuiver::GetLevel(
    const std::string& name
) const
{
    for(auto* level : fLevels)
    {
        if(level != nullptr &&
           level->GetName() == name)
        {
            return level;
        }
    }

    return nullptr;
}


const std::vector<DecayLevel*>&
DecayQuiver::GetLevels() const
{
    return fLevels;
}


// ============================================================
// Transitions
// ============================================================

DecayTransition* DecayQuiver::AddTransition(
    const std::string& name,
    DecayLevel* source,
    DecayLevel* target,
    double probability
)
{
    if(source == nullptr)
    {
        throw std::invalid_argument(
            "DecayTransition source cannot be nullptr."
        );
    }

    if(target == nullptr)
    {
        throw std::invalid_argument(
            "DecayTransition target cannot be nullptr."
        );
    }

    if(GetTransition(name) != nullptr)
    {
        throw std::invalid_argument(
            "Decay transition '" + name + "' already exists."
        );
    }

    auto* transition =
        new DecayTransition(
            name,
            source,
            target,
            probability
        );

    fTransitions.push_back(transition);

    return transition;
}


DecayTransition* DecayQuiver::GetTransition(
    const std::string& name
) const
{
    for(auto* transition : fTransitions)
    {
        if(transition != nullptr &&
           transition->GetName() == name)
        {
            return transition;
        }
    }

    return nullptr;
}


const std::vector<DecayTransition*>&
DecayQuiver::GetTransitions() const
{
    return fTransitions;
}

bool DecayQuiver::RemoveTransition(const DecayTransition* transition)
{
    if(transition == nullptr) return false;

    for(auto it = fTransitions.begin(); it != fTransitions.end(); ++it)
    {
        if(*it == transition)
        {
            delete *it;
            fTransitions.erase(it);
            return true;
        }
    }

    return false;
}

bool DecayQuiver::RemoveLevel(const DecayLevel* level)
{
    if(level == nullptr) return false;

    // remove transitions referring to this level
    for(auto it = fTransitions.begin(); it != fTransitions.end(); ) {
        DecayTransition* tr = *it;
        if(tr != nullptr && (tr->GetSource() == level || tr->GetTarget() == level)) {
            delete tr;
            it = fTransitions.erase(it);
        } else {
            ++it;
        }
    }

    // remove the level itself
    for(auto it = fLevels.begin(); it != fLevels.end(); ++it)
    {
        if(*it == level)
        {
            delete *it;
            fLevels.erase(it);
            return true;
        }
    }

    return false;
}


// ============================================================
// Quiver structure
// ============================================================

bool DecayQuiver::IsComposable(
    const DecayTransition* first,
    const DecayTransition* second
) const
{
    if(first == nullptr || second == nullptr)
    {
        return false;
    }

    return first->GetTarget() == second->GetSource();
}


bool DecayQuiver::HasDirectTransition(
    const DecayLevel* source,
    const DecayLevel* target
) const
{
    if(source == nullptr || target == nullptr)
    {
        return false;
    }

    for(auto* transition : fTransitions)
    {
        if(transition == nullptr)
        {
            continue;
        }

        if(transition->GetSource() == source &&
           transition->GetTarget() == target)
        {
            return true;
        }
    }

    return false;
}


// ============================================================
// Probabilities
// ============================================================

double DecayQuiver::GetOutgoingProbability(
    const DecayLevel* level
) const
{
    if(level == nullptr)
    {
        throw std::invalid_argument(
            "Cannot calculate outgoing probability "
            "for nullptr level."
        );
    }

    double total = 0.0;

    for(auto* transition : fTransitions)
    {
        if(transition == nullptr)
        {
            continue;
        }

        if(transition->GetSource() == level)
        {
            total += transition->GetProbability();
        }
    }

    return total;
}


bool DecayQuiver::IsNormalized(
    const DecayLevel* level,
    double tolerance
) const
{
    if(level == nullptr)
    {
        return false;
    }

    const double total =
        GetOutgoingProbability(level);

    return std::abs(total - 1.0) <= tolerance;
}


// ============================================================
// Printing
// ============================================================

void DecayQuiver::Print() const
{
    std::cout << std::endl;

    std::cout << "Vertices:" << std::endl;

    for(auto* level : fLevels)
    {
        if(level != nullptr)
        {
            level->Print();
        }
    }

    std::cout << std::endl;

    std::cout << "Arrows:" << std::endl;

    for(auto* transition : fTransitions)
    {
        if(transition != nullptr)
        {
            transition->Print();
        }
    }

    std::cout << std::endl;
}

namespace {
// JSON strings need control-character escaping; names are otherwise UTF-8.
std::string jsonString(const std::string& value)
{
    std::string result = "\"";
    const char* hex = "0123456789abcdef";
    for(unsigned char c : value)
    {
        if(c == '"' || c == '\\')
        {
            result += '\\';
            result += c;
        }
        else if(c < 0x20)
        {
            result += "\\u00";
            result += hex[c >> 4];
            result += hex[c & 15];
        }
        else result += c;
    }
    return result + '"';
}
}

void DecayQuiver::ExportJson(
    const std::string& filename,
    const std::vector<DecayVector>& vectors,
    const std::string& title,
    const std::unordered_map<std::string, double>& conversionCoefficients,
    const std::vector<double>& initialPopulations
) const
{
    const auto require = [](bool condition, const std::string& message) {
        if(!condition) throw std::invalid_argument("DecayQuiver::ExportJson: " + message);
    };
    const auto levelIndex = [&](const DecayLevel* level) {
        const auto it = std::find(fLevels.begin(), fLevels.end(), level);
        require(level != nullptr && it != fLevels.end(), "level does not belong to this quiver");
        return std::distance(fLevels.begin(), it);
    };
    const auto validName = [&](const std::string& name) {
        require(name.find_first_not_of(" \t\r\n\f\v") != std::string::npos,
                "names must not be blank");
    };

    // Validate and serialize before opening the destination.
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    out << "{\n  \"title\": " << jsonString(title) << ",\n  \"levels\": [";
    std::set<std::string> names;
    for(std::size_t i = 0; i < fLevels.size(); ++i)
    {
        require(fLevels[i] != nullptr, "null level");
        const auto& name = fLevels[i]->GetName();
        validName(name);
        require(names.insert(name).second, "duplicate level name: " + name);
        out << (i ? ", " : "") << jsonString(name);
    }
    out << "]";
    // Preserve the names array for older consumers; null denotes unknown energy.
    out << ",\n  \"level_energies_keV\": [";
    for (std::size_t i = 0; i < fLevels.size(); ++i)
    {
        if (i) out << ", ";
        if (fLevels[i]->HasEnergy()) out << fLevels[i]->GetEnergy();
        else out << "null";
    }
    out << "],\n  \"transitions\": [";
    names.clear();
    for(std::size_t i = 0; i < fTransitions.size(); ++i)
    {
        const auto* t = fTransitions[i];
        require(t != nullptr, "null transition");
        validName(t->GetName());
        require(names.insert(t->GetName()).second, "duplicate transition name: " + t->GetName());
        const double p = t->GetProbability();
        require(std::isfinite(p) && p >= 0 && p <= 1, "transition probability must be in [0, 1]");
        out << (i ? "," : "") << "\n    {\"name\": " << jsonString(t->GetName())
            << ", \"source_index\": " << levelIndex(t->GetSource())
            << ", \"target_index\": " << levelIndex(t->GetTarget())
            << ", \"probability\": " << p << "}";
    }
    out << "\n  ],\n  \"vectors\": [";
    for(std::size_t i = 0; i < vectors.size(); ++i)
    {
        out << (i ? "," : "") << "\n    {\"terms\": [";
        const auto& terms = vectors[i].GetTerms();
        for(std::size_t j = 0; j < terms.size(); ++j)
        {
            const auto& term = terms[j];
            require(std::isfinite(term.coefficient), "vector coefficient must be finite");
            levelIndex(term.path.GetSource());
            levelIndex(term.path.GetTarget());
            out << (j ? "," : "") << "\n      {\"path_names\": [";
            const auto& transitions = term.path.GetTransitions();
            for(std::size_t k = 0; k < transitions.size(); ++k)
            {
                const auto& t = transitions[k];
                const auto* original = GetTransition(t.GetName());
                require(original && original->GetSource() == t.GetSource()
                        && original->GetTarget() == t.GetTarget()
                        && original->GetProbability() == t.GetProbability(),
                        "vector transition does not match this quiver: " + t.GetName());
                out << (k ? ", " : "") << jsonString(t.GetName());
            }
            out << "]";
            if(term.path.IsStationary())
                out << ", \"stationary_level\": " << jsonString(term.path.GetSource()->GetName());
            out << ", \"coefficient\": " << term.coefficient << "}";
        }
        out << "\n    ]}";
    }
    out << "\n  ]";
    if (!conversionCoefficients.empty() || !initialPopulations.empty()) {
        out << ",\n  \"metadata\": {";
        bool comma=false;
        if (!conversionCoefficients.empty()) {
            require(conversionCoefficients.size()==fTransitions.size(), "IC map must cover every transition exactly");
            out << "\n    \"transition_probability_basis\": \"gamma_plus_ic\",\n    \"conversion_coefficients\": {";
            for(std::size_t i=0;i<fTransitions.size();++i) {
                const auto& name=fTransitions[i]->GetName();
                auto it=conversionCoefficients.find(name);
                require(it!=conversionCoefficients.end() && std::isfinite(it->second) && it->second>=0, "missing or invalid IC coefficient for " + name);
                out << (i?",":"") << "\n      " << jsonString(name) << ": " << it->second;
            }
            out << "\n    }"; comma=true;
        }
        if (!initialPopulations.empty()) {
            require(initialPopulations.size()==fLevels.size(), "initial populations must align with levels");
            double total=0;
            out << (comma?",":"") << "\n    \"branching\": [";
            for(std::size_t i=0;i<initialPopulations.size();++i) {
                double value=initialPopulations[i];
                require(std::isfinite(value) && value>=0 && value<=1, "invalid initial population");
                total+=value; out << (i?", ":"") << value;
            }
            require(std::abs(total-1)<1e-8, "initial populations must sum to one");
            out << "]";
        }
        out << "\n  }";
    }
    out << "\n}\n";
    std::ofstream file(filename);
    if(!file) throw std::runtime_error("DecayQuiver::ExportJson: cannot open " + filename);
    file << out.str();
    file.close();
    if(!file) throw std::runtime_error("DecayQuiver::ExportJson: failed to write " + filename);
}
