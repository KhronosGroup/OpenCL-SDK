#include "call_all.c"

int main(int argc, char** argv)
{
    // Purposefully always false.
    if (argc == 100) {
        call_all();
    }
    return 0;
}
