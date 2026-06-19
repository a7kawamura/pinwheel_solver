#include "pinwheel/solver_cli.hpp"
#include "pinwheel/policies.hpp"

int main() {
    pinwheel::run_solver<pinwheel::PackingPolicy>();
    return 0;
}
