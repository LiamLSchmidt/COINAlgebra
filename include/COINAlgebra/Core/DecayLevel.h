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

    const std::string& GetName() const;

    void SetName(
        const std::string& name
    );

    void Print() const;

private:

    std::string fName;
};

#endif
