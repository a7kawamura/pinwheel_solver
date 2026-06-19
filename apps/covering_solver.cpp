#include "pinwheel/solver_cli.hpp"
#include "pinwheel/policies.hpp"

int main() {
    pinwheel::run_solver<pinwheel::CoveringPolicy>();
    return 0;
}
