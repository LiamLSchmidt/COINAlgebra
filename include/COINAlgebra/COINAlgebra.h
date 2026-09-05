#ifndef COINALGEBRA_H
#define COINALGEBRA_H

#include <string>

class TRint;

class COINAlgebra
{
public:

    COINAlgebra();
    ~COINAlgebra();

    void PrintBanner() const;
    void PrintHelp() const;
    void PrintVersion() const;

    void ConfigureROOT(TRint& rootApp);

    void InitializeEnvironment(TRint& rootApp);

    const std::string& GetVersion() const;

private:

    std::string fVersion;
};

void COINAlgebraHelp();
void COINAlgebraVersion();
#endif
