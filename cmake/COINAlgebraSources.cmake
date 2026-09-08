# Explicit source list shared by standalone, ROOT and GUI builds.
set(COINALGEBRA_MODULE_FILES
    Core/DecayLevel
    Core/DecayTransition
    Core/DecayPath
    Core/DecayQuiver
    Core/DecayVector
    Algebra/PathAlgebra
    Algebra/PathProjectors
    Probability/DecayProbability
    Builders/DecayQuiverBuilder
    NuclearData/RadioactiveDecayReader
    NuclearData/PhotonEvaporationReader)
set(COINALGEBRA_SOURCES)
set(COINALGEBRA_HEADERS)
foreach(file IN LISTS COINALGEBRA_MODULE_FILES)
    list(APPEND COINALGEBRA_SOURCES "${PROJECT_SOURCE_DIR}/src/${file}.cxx")
    list(APPEND COINALGEBRA_HEADERS "${PROJECT_SOURCE_DIR}/include/COINAlgebra/${file}.h")
endforeach()
