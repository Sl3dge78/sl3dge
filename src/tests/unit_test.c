
#include <sl3dge-utils/sl3dge.h>

#include <stdio.h>

void TestHuffman() {
    sLog("HUFFMAN");
    u32 source[] = {3, 3, 3, 3, 3, 2, 4, 4};
    u32 expected[] = {0b010, 0b011, 0b100, 0b101, 0b110, 0b00, 0b1110, 0b1111};
    u32 out[ARRAY_SIZE(source)];
    HuffmanCompute(ARRAY_SIZE(source), source, out);

    for(u32 i = 0; i < ARRAY_SIZE(expected); ++i) {
        TEST_EQUALS(out[i], expected[i], "%X");
    }
}

void TestVec3() {
    {
        sLog("VEC3");

        Vec3 test_1 = {55222.65654, 5646654.0, 46584.321132};
        Vec3 test_2 = {1.0, 1.0, 1.0};

        Vec3 result = vec3_add(test_1, test_2);
        TEST_EQUALS(result.x, test_1.x + test_2.x, "%.2f");
        TEST_EQUALS(result.y, test_1.y + test_2.y, "%.2f");
        TEST_EQUALS(result.z, test_1.z + test_2.z, "%.2f");

        result = vec3_sub(test_1, test_2);
        TEST_EQUALS(result.x, test_1.x - test_2.x, "%.2f");
        TEST_EQUALS(result.y, test_1.y - test_2.y, "%.2f");
        TEST_EQUALS(result.z, test_1.z - test_2.z, "%.2f");

        f32 value = 4343949.4934939f;
        result = vec3_fmul(test_1, value);
        TEST_EQUALS(result.x, test_1.x * value, "%.2f");
        TEST_EQUALS(result.y, test_1.y * value, "%.2f");
        TEST_EQUALS(result.z, test_1.z * value, "%.2f");

        result = vec3_normalize(result);
        f32 length = vec3_length(result);
        TEST_EQUALS(length, 1.0f, "%.2f");

        Vec3 test_3 = {1.0f, 0.0f, 0.0f};
        length = vec3_length(test_3);
        TEST_EQUALS(length, 1.0f, "%.2f");

        result = vec3_fmul(test_3, 2.0f);
        length = vec3_length(result);
        TEST_EQUALS(length, 2.0f, "%.2f");
    }
}

int main(const int argc, const char *argv[]) {
    TEST_BEGIN();
    TestVec3();
    TestHuffman();

    TEST_END();

    return 0;
}