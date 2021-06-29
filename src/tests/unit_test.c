
#include <sl3dge-utils/sl3dge.h>

#include <stdio.h>

int main(const int argc, const char *argv[]) {
    TEST_BEGIN();

    Vec3 test_1 = {0.0, 0.0, 0.0};
    Vec3 test_2 = {1.0, 1.0, 1.0};

    Vec3 result = vec3_add(test_1, test_2);
    TEST_EQUALS(result.x, test_1.x + test_2.x + 1);
    TEST_EQUALS(result.y, test_1.y + test_2.y);
    TEST_EQUALS(result.z, test_1.z + test_2.z);

    TEST_END();

    return 0;
}