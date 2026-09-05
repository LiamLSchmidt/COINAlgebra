#include <TSystem.h>
#include <TROOT.h>

#include <iostream>


void COINAlgebraLogon()
{
    std::cout << std::endl;

    std::cout
        << "Loading COINAlgebra environment..."
        << std::endl;

    std::cout << std::endl;


    // --------------------------------------------------------
    // Load COINAlgebra library
    // --------------------------------------------------------

    if(gSystem->Load("libCOINAlgebra.so") < 0)
    {
        std::cerr
            << "ERROR: Failed to load libCOINAlgebra.so"
            << std::endl;

        return;
    }


    // --------------------------------------------------------
    // Include the COINAlgebra command declarations
    // --------------------------------------------------------

    gROOT->ProcessLine(
        "#include \"COINAlgebra/Commands.h\""
    );


    // --------------------------------------------------------
    // Define convenient interactive commands
    // --------------------------------------------------------

    gROOT->ProcessLine(
        "void help() { COINAlgebraHelp(); }"
    );

    gROOT->ProcessLine(
        "void version() { COINAlgebraVersion(); }"
    );


    std::cout
        << "COINAlgebra library loaded."
        << std::endl;

    std::cout << std::endl;
}
