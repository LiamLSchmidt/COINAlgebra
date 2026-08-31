#include "COINAlgebra/COINAlgebra.h"

#include <TRint.h>
#include <TSystem.h>

#include <cstdlib>
#include <iostream>

COINAlgebra::COINAlgebra()
    : fVersion("0.1.0")
{
}

COINAlgebra::~COINAlgebra()
{
}

void COINAlgebra::PrintBanner() const
{
    std::cout << std::endl;

    std::cout
        << "============================================================"
        << std::endl;

    std::cout
        << "                      COINAlgebra"
        << std::endl;

    std::cout
        << "       Coincidence Algebra and Decay Quiver Toolkit"
        << std::endl;

    std::cout
        << "============================================================"
        << std::endl;

    std::cout << std::endl;
}

void COINAlgebra::PrintVersion() const
{
    std::cout
        << "COINAlgebra version "
        << fVersion
        << std::endl;
}

void COINAlgebra::PrintHelp() const
{
    std::cout << std::endl;

    std::cout << "COINAlgebra commands:"
              << std::endl;

    std::cout << std::endl;

    std::cout << "  help"
              << "              Display this help message"
              << std::endl;

    std::cout << "  version"
              << "           Display COINAlgebra version"
              << std::endl;

    std::cout << "  quit"
              << "              Exit COINAlgebra"
              << std::endl;

    std::cout << "  exit"
              << "              Exit COINAlgebra"
              << std::endl;

    std::cout << std::endl;

    std::cout << "ROOT commands:"
              << std::endl;

    std::cout << std::endl;

    std::cout << "  .L <file>"
              << "         Load a ROOT macro or library"
              << std::endl;

    std::cout << "  .x <file>"
              << "         Execute a ROOT macro"
              << std::endl;

    std::cout << "  .q"
              << "                Quit"
              << std::endl;

    std::cout << std::endl;
}

void COINAlgebra::ConfigureROOT(TRint& rootApp)
{
    //
    // Set the interactive prompt.
    //
    // %d is replaced by ROOT with the command number.
    //

    rootApp.SetPrompt("COINAlgebra [%d]> ");
}

const std::string& COINAlgebra::GetVersion() const
{
    return fVersion;
}

void COINAlgebra::InitializeEnvironment(TRint& rootApp)
{
    const char* home = std::getenv("COINALGEBRA_HOME");

    if(home == nullptr)
    {
        std::cerr
            << "WARNING: COINALGEBRA_HOME is not set."
            << std::endl;

        std::cerr
            << "COINAlgebra startup environment could not be initialized."
            << std::endl;

        return;
    }

    std::string configDirectory = std::string(home) + "/config";

    std::string logonMacro =
        configDirectory + "/COINAlgebraLogon.C";

    if(gSystem->AccessPathName(logonMacro.c_str()))
    {
        std::cerr
            << "WARNING: COINAlgebra logon macro not found:"
            << std::endl;

        std::cerr
            << "  "
            << logonMacro
            << std::endl;

        return;
    }

    std::cout
        << "Loading COINAlgebra environment..."
        << std::endl;

    rootApp.ProcessLine(
        (".x " + logonMacro).c_str()
    );
}

