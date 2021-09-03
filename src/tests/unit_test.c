
#include <sl3dge-utils/sl3dge.h>

#include <stdio.h>
#include "collision.h"

void TestHuffman() {
    // @Test This test code isn't valid anymore
    sLog("HUFFMAN");
    u32 source[] = {3, 3, 3, 3, 3, 2, 4, 4};
    u32 expected[] = {0b010, 0b011, 0b100, 0b101, 0b110, 0b00, 0b1110, 0b1111};
    u32 out[ARRAY_SIZE(source)];
    HuffmanCompute(ARRAY_SIZE(source), source, out);

    for(u32 i = 0; i < ARRAY_SIZE(expected); ++i) {
        TEST_EQUALS(out[i], expected[i], "%X");
    }
    u8 code[] = {0b00011010, 0b01111111};
    u32 decoded[] = {0, 4, 5, 7, 6};
    PNG_DataStream stream = {};
    stream.contents = &code;
    stream.contents_size = 2;
    u32 result[ARRAY_SIZE(decoded)];
    for(u32 i = 0; i < ARRAY_SIZE(decoded); ++i) {
        result[i] = HuffmanDecode(&stream, expected, source, ARRAY_SIZE(source));
        TEST_EQUALS(result[i], decoded[i], "%X");
    }

    sLog("");
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
        sLog("");
    }
}

void TestMat() {
    sLog("MAT");

    Vec3 scale = (Vec3){2.3f, 5.6f, 0.4f};
    Mat4 mat = mat4_identity();
    mat4_scaleby(&mat, scale);

    Vec3 result = mat4_get_scale(&mat);
    TEST_EQUALS(scale.x, result.x, "%.2f");
    TEST_EQUALS(scale.y, result.y, "%.2f");
    TEST_EQUALS(scale.y, result.y, "%.2f");
    sLog("");
}

int main(const int argc, const char *argv[]) {
    Leak_Begin();
    TEST_BEGIN();
    sLogLevel(LOG_LEVEL_LOG);

    TestVec3();

    u8 byte = 0b10110001;
    u8 result = swap_u8(byte);
    TEST_EQUALS(result, 0b10001101, "%X");

    //TestHuffman();
    TestMat();

    TESTCOLLISION();

    TEST_END();
    Leak_End();

    return 0;
}