#ifndef COINALGEBRA_DECAYLEVEL_H
#define COINALGEBRA_DECAYLEVEL_H

#include <string>

class DecayLevel
{
public:

    DecayLevel();

    explicit DecayLevel(
        const std::string& name
    );

    DecayLevel(const std::string& name, double energy_keV);

    // Excitation energy in keV. Unknown is distinct from the ground state's 0.
    // GetEnergy throws logic_error if unset; setters reject nonfinite/negative values.
    bool HasEnergy() const;
    double GetEnergy() const;
    void SetEnergy(double energy_keV);
    void ClearEnergy();

    const std::string& GetName() const;

    void SetName(
        const std::string& name
    );

    void Print() const;

private:

    std::string fName;
    double fEnergy = 0.0;
    bool fHasEnergy = false;
};

#endif
