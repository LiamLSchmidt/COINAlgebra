#include "COINAlgebra/Core/DecayVector.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>


DecayVector::DecayVector()
{
}


DecayVector::DecayVector(
    const DecayPath& path,
    double coefficient
)
{
    AddTerm(path, coefficient);
}


// ============================================================
// Properties
// ============================================================

bool DecayVector::Empty() const
{
    return fTerms.empty();
}


std::size_t DecayVector::Size() const
{
    return fTerms.size();
}


const std::vector<DecayVector::Term>&
DecayVector::GetTerms() const
{
    return fTerms;
}


// ============================================================
// Vector construction
// ============================================================

void DecayVector::AddTerm(
    const DecayPath& path,
    double coefficient
)
{
    if(path.Empty())
    {
        throw std::invalid_argument(
            "DecayVector::AddTerm: "
            "cannot add an empty path."
        );
    }

    if(coefficient == 0.0)
    {
        return;
    }

    for(auto& term : fTerms)
    {
        if(PathsEqual(term.path, path))
        {
            term.coefficient += coefficient;

            Simplify();

            return;
        }
    }

    fTerms.push_back(
        {path, coefficient}
    );
}


// ============================================================
// Addition
// ============================================================

DecayVector DecayVector::operator+(
    const DecayVector& other
) const
{
    DecayVector result = *this;

    result += other;

    return result;
}


DecayVector& DecayVector::operator+=(
    const DecayVector& other
)
{
    for(const auto& term : other.fTerms)
    {
        AddTerm(
            term.path,
            term.coefficient
        );
    }

    return *this;
}


// ============================================================
// Subtraction
// ============================================================

DecayVector DecayVector::operator-(
    const DecayVector& other
) const
{
    DecayVector result = *this;

    result -= other;

    return result;
}


DecayVector& DecayVector::operator-=(
    const DecayVector& other
)
{
    for(const auto& term : other.fTerms)
    {
        AddTerm(
            term.path,
            -term.coefficient
        );
    }

    return *this;
}


// ============================================================
// Scalar multiplication
// ============================================================

DecayVector DecayVector::operator*(
    double scalar
) const
{
    DecayVector result = *this;

    result *= scalar;

    return result;
}


DecayVector& DecayVector::operator*=(
    double scalar
)
{
    for(auto& term : fTerms)
    {
        term.coefficient *= scalar;
    }

    Simplify();

    return *this;
}


DecayVector operator*(
    double scalar,
    const DecayVector& vector
)
{
    return vector * scalar;
}


// ============================================================
// Path comparison
// ============================================================

bool DecayVector::PathsEqual(
    const DecayPath& first,
    const DecayPath& second
)
{
    return first == second;
}

// ============================================================
// Simplification
// ============================================================

void DecayVector::Simplify()
{
    constexpr double tolerance = 1e-14;

    std::vector<Term> simplified;

    for(const auto& term : fTerms)
    {
        if(
            term.coefficient >
                tolerance ||
            term.coefficient <
                -tolerance
        )
        {
            simplified.push_back(term);
        }
    }

    fTerms = std::move(simplified);
}


// ============================================================
// Display
// ============================================================

std::string DecayVector::ToString() const
{
    if(fTerms.empty())
    {
        return "0";
    }

    std::ostringstream stream;

    for(std::size_t i = 0;
        i < fTerms.size();
        ++i)
    {
        const auto& term = fTerms[i];

        if(i > 0)
        {
            if(term.coefficient >= 0.0)
            {
                stream << " + ";
            }
            else
            {
                stream << " - ";
            }
        }
        else if(term.coefficient < 0.0)
        {
            stream << "-";
        }

        double magnitude =
            term.coefficient < 0.0
                ? -term.coefficient
                : term.coefficient;

        stream
            << magnitude
            << " * ("
            << term.path.ToString()
            << ")";
    }

    return stream.str();
}


void DecayVector::Print() const
{
    std::cout
        << ToString()
        << std::endl;
}

// ============================================================
// Table display
// ============================================================
//
// Displays each path and its coefficient interpreted as a
// probability:
//
//     p_i = a_i
//
// Example:
//
//     +----------------------+--------------+
//     | Path                 | Probability  |
//     +----------------------+--------------+
//     | d2 -> d1             | 0.350000     |
//     | d2 -> d0             | 0.150000     |
//     +----------------------+--------------+
//
// ============================================================

void DecayVector::PrintTable() const
{
    PrintTable(std::cout);
}

void DecayVector::PrintTable(std::ostream& out) const
{
    if(fTerms.empty())
    {
        out << "DecayVector is empty."
                  << std::endl;

        return;
    }


    // --------------------------------------------------------
    // Determine the required path-column width.
    // --------------------------------------------------------

    std::size_t pathWidth = 4;

    for(const auto& term : fTerms)
    {
        const std::string pathString =
            term.path.ToString();

        if(pathString.length() > pathWidth)
        {
            pathWidth = pathString.length();
        }
    }


    // --------------------------------------------------------
    // Minimum width for a readable table.
    // --------------------------------------------------------

    if(pathWidth < 20)
    {
        pathWidth = 20;
    }


    const std::size_t probabilityWidth = 14;


    // --------------------------------------------------------
    // Horizontal separator.
    // --------------------------------------------------------

    out
        << "+"
        << std::string(pathWidth + 2, '-')
        << "+"
        << std::string(probabilityWidth + 2, '-')
        << "+"
        << std::endl;


    // --------------------------------------------------------
    // Header.
    // --------------------------------------------------------

    out
        << "| "
        << std::left
        << std::setw(static_cast<int>(pathWidth))
        << "Path"
        << " | "
        << std::right
        << std::setw(static_cast<int>(probabilityWidth))
        << "Probability"
        << " |"
        << std::endl;


    // --------------------------------------------------------
    // Header separator.
    // --------------------------------------------------------

    out
        << "+"
        << std::string(pathWidth + 2, '-')
        << "+"
        << std::string(probabilityWidth + 2, '-')
        << "+"
        << std::endl;


    // --------------------------------------------------------
    // Table contents.
    // --------------------------------------------------------

    for(const auto& term : fTerms)
    {
        const std::string pathString =
            term.path.ToString();

        out
            << "| "
            << std::left
            << std::setw(static_cast<int>(pathWidth))
            << pathString
            << " | "
            << std::right
            << std::fixed
            << std::setprecision(8)
            << std::setw(static_cast<int>(probabilityWidth))
            << term.coefficient
            << " |"
            << std::endl;
    }


    // --------------------------------------------------------
    // Bottom separator.
    // --------------------------------------------------------

    out
        << "+"
        << std::string(pathWidth + 2, '-')
        << "+"
        << std::string(probabilityWidth + 2, '-')
        << "+"
        << std::endl;
}

// --------------------------------------------------------
// Detection map
// --------------------------------------------------------

DecayVector DecayVector::ApplyDetectionMap(
    const std::unordered_map<std::string, double>& efficiencies
) const
{
    DecayVector result;


    for (const auto& term : fTerms)
    {
        const DecayPath& path = term.path;

        double pathEfficiency = 1.0;


        // ----------------------------------------------------
        // Stationary paths
        // ----------------------------------------------------
        //
        // A stationary path contains no physical transition.
        //
        // Therefore:
        //
        //      epsilon(e_v) = 1.
        //
        // Its coefficient is left unchanged.
        // ----------------------------------------------------

        if (!path.IsStationary())
        {
            const auto& transitions =
                path.GetTransitions();


            // ------------------------------------------------
            // Non-stationary paths
            // ------------------------------------------------
            //
            // For
            //
            //      p = gamma_1 ... gamma_n
            //
            // calculate
            //
            //      epsilon(p)
            //          = product_i epsilon(gamma_i).
            // ------------------------------------------------

            for (const auto& transition : transitions)
            {
                const std::string& transitionName =
                    transition.GetName();

                const auto efficiencyIt =
                    efficiencies.find(transitionName);


                if (efficiencyIt == efficiencies.end())
                {
                    throw std::runtime_error(
                        "DecayVector::ApplyDetectionMap: "
                        "No detection efficiency provided for "
                        "transition '" +
                        transitionName +
                        "'."
                    );
                }


                const double efficiency =
                    efficiencyIt->second;


                if (!std::isfinite(efficiency))
                {
                    throw std::runtime_error(
                        "DecayVector::ApplyDetectionMap: "
                        "Detection efficiency for transition '" +
                        transitionName +
                        "' is not finite."
                    );
                }


                if (efficiency < 0.0 ||
                    efficiency > 1.0)
                {
                    throw std::runtime_error(
                        "DecayVector::ApplyDetectionMap: "
                        "Detection efficiency for transition '" +
                        transitionName +
                        "' must lie between 0 and 1."
                    );
                }


                pathEfficiency *= efficiency;
            }
        }


        // ----------------------------------------------------
        // Apply detection factor to coefficient
        // ----------------------------------------------------

        result.AddTerm(
            path,
            term.coefficient * pathEfficiency
        );
    }


    return result;
}
DecayVector DecayVector::ApplyConversionMap(
    const std::unordered_map<std::string, double>& conversionCoefficients
) const
{
    std::unordered_map<std::string, double> emissionFactors;
    for (const auto& term : fTerms)
        for (const auto& transition : term.path.GetTransitions())
        {
            const auto& name = transition.GetName();
            const auto entry = conversionCoefficients.find(name);
            if (entry == conversionCoefficients.end())
                throw std::invalid_argument("DecayVector::ApplyConversionMap: missing coefficient for " + name);
            const double alpha = entry->second;
            if (!std::isfinite(alpha) || alpha < 0.0)
                throw std::invalid_argument("DecayVector::ApplyConversionMap: invalid coefficient for " + name);
            emissionFactors[name] = 1.0 / (1.0 + alpha);
        }
    return ApplyDetectionMap(emissionFactors);
}
