#include "shared_library.hpp"
#include "parse_arguments.hpp"

using namespace std;

int main(int argc, char *argv[]) {
    // process arguments
    Options opt = parse_arguments(argc, argv, true);

    if (opt.fork)
        client_fork(opt);
    else
        client_nofork(opt);
    return 0;
}
