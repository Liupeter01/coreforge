#include <common.cuh>

INSTANTIATE_TEST_SUITE_P(
    TransposeSizes, TestStallShortTestSuite,
    ::testing::Values(
        TransposeParam{32, 32}, TransposeParam{64, 32}, TransposeParam{32, 64},

        TransposeParam{31, 17}, TransposeParam{33, 65}, TransposeParam{65, 33},

        TransposeParam{1000, 777}, TransposeParam{777, 1000},

        TransposeParam{16384, 8192}, TransposeParam{8192, 16384}));
