#include "test_reader_gamma_quiver.C"
#include <stdexcept>

// ROOT -q uses a macro's integer/bool result as its process exit status.
// Convert true-on-success into a normal successful void macro completion.
void test_reader_gamma_quiver_smoke()
{
    if (!test_reader_gamma_quiver(12, 22, 11, 22))
        throw std::runtime_error("Reader-built gamma quiver validation failed");
}
