#include "COINAlgebra/Core/DecayLevel.h"

#include <iostream>
#include <cmath>
#include <stdexcept>

DecayLevel::DecayLevel()
    : fName("")
{
}

DecayLevel::DecayLevel(
    const std::string& name
)
    : fName(name)
{
}

DecayLevel::DecayLevel(const std::string& name, double energy_keV)
    : fName(name)
{
    SetEnergy(energy_keV);
}

bool DecayLevel::HasEnergy() const
{
    return fHasEnergy;
}

double DecayLevel::GetEnergy() const
{
    if (!fHasEnergy)
        throw std::logic_error("Energy is not set for decay level '" + fName + "'.");
    return fEnergy;
}

void DecayLevel::SetEnergy(double energy_keV)
{
    if (!std::isfinite(energy_keV) || energy_keV < 0.0)
        throw std::invalid_argument("Decay level energy must be finite and nonnegative (keV).");
    fEnergy = energy_keV;
    fHasEnergy = true;
}

void DecayLevel::ClearEnergy()
{
    fEnergy = 0.0;
    fHasEnergy = false;
}

const std::string& DecayLevel::GetName() const
{
    return fName;
}

void DecayLevel::SetName(
    const std::string& name
)
{
    fName = name;
}

void DecayLevel::Print() const
{
    std::cout
        << "Vertex(name='"
        << fName
        << "'";
    if (fHasEnergy) std::cout << ", energy=" << fEnergy << " keV";
    std::cout << ")" << std::endl;
}
