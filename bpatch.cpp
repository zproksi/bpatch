#include "stdafx.h"
#include "processing.h"

int main(int argc, char* argv[])
{
    return bpatch::Processing(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE;
}
