#include "COINAlgebra/COINAlgebra.h"

#include <TRint.h>

#include <iostream>


int main(int argc, char** argv)
{
    COINAlgebra app;


    // --------------------------------------------------------
    // Banner
    // --------------------------------------------------------

    app.PrintBanner();


    // --------------------------------------------------------
    // ROOT interactive application
    // --------------------------------------------------------

    TRint rootApp(
        "COINAlgebra",
        &argc,
        argv,
        nullptr,
        0,
        true
    );


    // --------------------------------------------------------
    // Configure ROOT
    // --------------------------------------------------------

    app.ConfigureROOT(rootApp);


    // --------------------------------------------------------
    // Version
    // --------------------------------------------------------

    app.PrintVersion();

    std::cout << std::endl;


    // --------------------------------------------------------
    // Initialize environment
    // --------------------------------------------------------

    app.InitializeEnvironment(rootApp);

    std::cout << std::endl;


    // --------------------------------------------------------
    // Welcome message
    // --------------------------------------------------------

    std::cout
        << "Welcome to the COINAlgebra interactive environment."
        << std::endl;

    std::cout << std::endl;

    std::cout
        << "Type 'help()' for COINAlgebra commands."
        << std::endl;

    std::cout << std::endl;


    // --------------------------------------------------------
    // Start ROOT
    // --------------------------------------------------------

    rootApp.Run();


    return 0;
}
