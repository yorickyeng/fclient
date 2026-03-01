#include <jni.h>
#include <string>

#include <android/log.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/android_sink.h>

#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/des.h>

#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, "fclient_ndk", __VA_ARGS__)
#define SLOG_INFO(...) android_logger->info( __VA_ARGS__ )
auto android_logger = spdlog::android_logger_mt("android", "fclient_ndk");

mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
const char *personalization = "fclient-sample-app";

extern "C" JNIEXPORT jstring JNICALL
Java_ru_iu3_fclient_MainActivity_stringFromJNI(JNIEnv *env, jobject /* thiz */) {
    spdlog::set_pattern("%v");

    std::string hello = "Hello from C++";

    LOG_INFO("Hello from logcat c++ %d", 2022);
    SLOG_INFO("Hello from spdlog {0}", 1967);

    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_ru_iu3_fclient_MainActivity_initRng(JNIEnv *env, jclass /* clazz */) {
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    return mbedtls_ctr_drbg_seed(&ctr_drbg,
                                 mbedtls_entropy_func,
                                 &entropy,
                                 (const unsigned char *) personalization,
                                 strlen(personalization));
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_MainActivity_randomBytes(JNIEnv *env, jclass, jint len) {
    auto *nativeBytes = new uint8_t[len];
    mbedtls_ctr_drbg_random(&ctr_drbg, nativeBytes, len);

    jbyteArray javaBytes = env->NewByteArray(len);
    env->SetByteArrayRegion(javaBytes, 0, len, (jbyte *) nativeBytes);

    delete[] nativeBytes;
    return javaBytes;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_MainActivity_encrypt(JNIEnv *env, jclass, jbyteArray key, jbyteArray data) {
    jsize keyLen = env->GetArrayLength(key);
    jsize dataLen = env->GetArrayLength(data);
    if ((keyLen != 16) || (dataLen <= 0)) return env->NewByteArray(0);

    mbedtls_des3_context ctx;
    mbedtls_des3_init(&ctx);

    jbyte *pkey = env->GetByteArrayElements(key, nullptr);
    jbyte *pdata = env->GetByteArrayElements(data, nullptr);

    // Паддинг PKCS#5
    int padding = 8 - (dataLen % 8);
    int len = dataLen + padding;
    auto *buf = new uint8_t[len];
    std::copy(pdata, pdata + dataLen, buf);
    for (int i = 0; i < padding; ++i)
        buf[dataLen + i] = padding;

    mbedtls_des3_set2key_enc(&ctx, (uint8_t *) pkey);

    int blockCount = len / 8;
    for (int i = 0; i < blockCount; i++)
        mbedtls_des3_crypt_ecb(&ctx, buf + i * 8, buf + i * 8);

    jbyteArray javaBytes = env->NewByteArray(len);
    env->SetByteArrayRegion(javaBytes, 0, len, (jbyte *) buf);

    delete[] buf;
    env->ReleaseByteArrayElements(key, pkey, JNI_ABORT);
    env->ReleaseByteArrayElements(data, pdata, JNI_ABORT);
    mbedtls_des3_free(&ctx);

    return javaBytes;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_MainActivity_decrypt(JNIEnv *env, jclass, jbyteArray key, jbyteArray data) {
    jsize keyLen = env->GetArrayLength(key);
    jsize dataLen = env->GetArrayLength(data);
    if ((keyLen != 16) || (dataLen <= 0) || ((dataLen % 8) != 0)) return env->NewByteArray(0);
    
    mbedtls_des3_context ctx;
    mbedtls_des3_init(&ctx);

    jbyte *pkey = env->GetByteArrayElements(key, nullptr);
    jbyte *pdata = env->GetByteArrayElements(data, nullptr);

    auto *buf = new uint8_t[dataLen];
    std::copy(pdata, pdata + dataLen, buf);
    
    mbedtls_des3_set2key_dec(&ctx, (uint8_t *) pkey);
    
    int blockCount = dataLen / 8;
    for (int i = 0; i < blockCount; i++)
        mbedtls_des3_crypt_ecb(&ctx, buf + i * 8, buf + i * 8);

    int padding = buf[dataLen - 1];
    int len = dataLen - padding;

    jbyteArray javaBytes = env->NewByteArray(len);
    env->SetByteArrayRegion(javaBytes, 0, len, (jbyte *) buf);
    
    delete[] buf;
    env->ReleaseByteArrayElements(key, pkey, JNI_ABORT);
    env->ReleaseByteArrayElements(data, pdata, JNI_ABORT);
    mbedtls_des3_free(&ctx);
    
    return javaBytes;
}