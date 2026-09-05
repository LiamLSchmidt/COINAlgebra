#include "COINAlgebra/DecayLevel.h"

#include <iostream>

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
        << "')"
        << std::endl;
}
