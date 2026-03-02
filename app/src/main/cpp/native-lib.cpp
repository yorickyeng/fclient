#include <jni.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <spdlog/sinks/android_sink.h>
#include <spdlog/spdlog.h>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/des.h>
#include <mbedtls/entropy.h>

namespace {
    // jvm
    JavaVM *gJvm = nullptr;

    // mbedtls rng
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *personalization = "fclient-sample-app";

    // pin state
    std::mutex pinMutex;
    std::condition_variable pinCv;
    std::string receivedPin;
    bool pinReady = false;

    // block state
    std::chrono::steady_clock::time_point blockedUntil;
    bool isBlocked = false;

    // env
    void releaseEnv(bool detach) { if (detach) gJvm->DetachCurrentThread(); }

    JNIEnv* getEnv(bool& detach) {
        JNIEnv* env = nullptr;
        detach = false;

        int status = gJvm->GetEnv((void**)&env, JNI_VERSION_1_6);

        if (status == JNI_EDETACHED) {
            if (gJvm->AttachCurrentThread(&env, nullptr) != 0)
                return nullptr;
            detach = true;
        }

        return env;
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    gJvm = vm;
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_ru_iu3_fclient_presentation_MainActivity_spdlog(JNIEnv *env, jclass, jstring jtag, jstring jmessage) {

    if (jmessage == nullptr) return;

    const char *msg = env->GetStringUTFChars(jmessage, nullptr);
    if (msg == nullptr) return;

    const char *tagChars = nullptr;
    if (jtag != nullptr) {
        tagChars = env->GetStringUTFChars(jtag, nullptr);
        if (tagChars == nullptr) { // OOM
            env->ReleaseStringUTFChars(jmessage, msg);
            return;
        }
    }
    const char *tag = (tagChars != nullptr) ? tagChars : "fclient_ndk";

    auto sink = std::make_shared<spdlog::sinks::android_sink_mt>(tag);
    spdlog::logger logger("tmp", sink);
    logger.set_pattern("%v");
    logger.info("{}", msg);

    env->ReleaseStringUTFChars(jmessage, msg);
    if (tagChars != nullptr) env->ReleaseStringUTFChars(jtag, tagChars);
}

extern "C" JNIEXPORT jint JNICALL
Java_ru_iu3_fclient_presentation_MainActivity_initRng(JNIEnv *env, jclass /* clazz */) {

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    return mbedtls_ctr_drbg_seed(&ctr_drbg,
                                 mbedtls_entropy_func,
                                 &entropy,
                                 (const unsigned char *) personalization,
                                 strlen(personalization));
}

extern "C" JNIEXPORT void JNICALL
Java_ru_iu3_fclient_presentation_MainActivity_freeRng(JNIEnv *env, jclass) {

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_presentation_MainActivity_randomBytes(JNIEnv *env, jclass, jint len) {

    auto *nativeBytes = new uint8_t[len];
    mbedtls_ctr_drbg_random(&ctr_drbg, nativeBytes, len);

    jbyteArray javaBytes = env->NewByteArray(len);
    env->SetByteArrayRegion(javaBytes, 0, len, (jbyte *) nativeBytes);

    delete[] nativeBytes;
    return javaBytes;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_ru_iu3_fclient_presentation_MainActivity_encrypt(JNIEnv *env, jclass, jbyteArray key, jbyteArray data) {

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
Java_ru_iu3_fclient_presentation_MainActivity_decrypt(JNIEnv *env, jclass, jbyteArray key, jbyteArray data) {

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

extern "C" JNIEXPORT void JNICALL
Java_ru_iu3_fclient_presentation_MainActivity_providePin(JNIEnv *env, jobject, jstring jpin) {

    if (!jpin) return;
    const char* utf = env->GetStringUTFChars(jpin, nullptr);
    if (!utf) return;

    {
        std::lock_guard<std::mutex> lock(pinMutex);
        receivedPin = utf;
        pinReady = true;
    }

    env->ReleaseStringUTFChars(jpin, utf);
    pinCv.notify_one();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_ru_iu3_fclient_presentation_MainActivity_transaction(JNIEnv *env, jobject thiz, jbyteArray trd) {

    jobject gThiz = env->NewGlobalRef(thiz);
    auto gTrd = (jbyteArray) env->NewGlobalRef(trd);

    std::thread([gThiz, gTrd]() {

        bool detach = false;
        JNIEnv *env = getEnv(detach);
        if (!env) return;

        jclass cls = env->GetObjectClass(gThiz);

        jmethodID enterPinId =
                env->GetMethodID(cls,
                                 "enterPin",
                                 "(ILjava/lang/String;)V");

        jmethodID resultId =
                env->GetMethodID(cls,
                                 "transactionResult",
                                 "(Z)V");

        // ----- parse TRD -----

        jsize sz = env->GetArrayLength(gTrd);
        jbyte *p = env->GetByteArrayElements(gTrd, nullptr);

        if (sz != 9) {
            env->CallVoidMethod(gThiz, resultId, false);
        } else {

            char buf[13];
            for (int i = 0; i < 6; i++) {
                uint8_t n = *(p + 3 + i);
                buf[i * 2] = ((n & 0xF0) >> 4) + '0';
                buf[i * 2 + 1] = (n & 0x0F) + '0';
            }
            buf[12] = 0;

            jstring jamount = env->NewStringUTF(buf);

            int attempts = 3;
            bool ok = false;

            auto now = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lock(pinMutex);
                if (isBlocked && now < blockedUntil) {
                    env->CallVoidMethod(gThiz, resultId, false);
                    goto cleanup;
                }
                isBlocked = false;
            }

            while (attempts > 0) {

                {
                    std::lock_guard<std::mutex> lock(pinMutex);
                    pinReady = false;
                }

                env->CallVoidMethod(gThiz, enterPinId, attempts, jamount);

                std::unique_lock<std::mutex> lock(pinMutex);
                pinCv.wait(lock, [] { return pinReady; });

                if (receivedPin == "1234") {
                    ok = true;
                    break;
                }

                attempts--;
            }

            if (!ok) {
                std::lock_guard<std::mutex> lock(pinMutex);
                isBlocked = true;
                blockedUntil = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            }

            env->CallVoidMethod(gThiz, resultId, ok);

            cleanup:

            env->DeleteLocalRef(jamount);
        }

        env->ReleaseByteArrayElements(gTrd, p, JNI_ABORT);
        env->DeleteLocalRef(cls);
        env->DeleteGlobalRef(gThiz);
        env->DeleteGlobalRef(gTrd);

        releaseEnv(detach);
    }).detach();

    return true;
}