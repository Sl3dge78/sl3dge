
#include <sl3dge-utils/sl3dge.h>

#include <stdio.h>

int main(const int argc, const char *argv[]) {
    TEST_BEGIN();

    sLog("VEC3");

    Vec3 test_1 = {55222.65654, 5646654.0, 46584.321132};
    Vec3 test_2 = {1.0, 1.0, 1.0};

    Vec3 result = vec3_add(test_1, test_2);
    TEST_EQUALS(result.x, test_1.x + test_2.x);
    TEST_EQUALS(result.y, test_1.y + test_2.y);
    TEST_EQUALS(result.z, test_1.z + test_2.z);

    result = vec3_sub(test_1, test_2);
    TEST_EQUALS(result.x, test_1.x - test_2.x);
    TEST_EQUALS(result.y, test_1.y - test_2.y);
    TEST_EQUALS(result.z, test_1.z - test_2.z);

    f32 value = 4343949.4934939f;
    result = vec3_fmul(test_1, value);
    TEST_EQUALS(result.x, test_1.x * value);
    TEST_EQUALS(result.y, test_1.y * value);
    TEST_EQUALS(result.z, test_1.z * value);

    result = vec3_normalize(result);
    f32 length = vec3_length(result);
    TEST_EQUALS(length, 1.0f);

    Vec3 test_3 = {1.0f, 0.0f, 0.0f};
    length = vec3_length(test_3);
    TEST_EQUALS(length, 1.0f);

    result = vec3_fmul(test_3, 2.0f);
    length = vec3_length(result);
    TEST_EQUALS(length, 2.0f);

    TEST_END();

    return 0;
}