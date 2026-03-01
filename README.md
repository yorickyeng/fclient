# fclient — Android NDK (JNI) + spdlog + mbedTLS

Учебное Android-приложение, в котором показано, как:
- вызывать C++ код из Kotlin через **JNI** (Android NDK);
- логировать из C++ в **Logcat**:
  - напрямую через `__android_log_print` (liblog)
  - и через **spdlog** с `android_sink`;
- использовать **mbedTLS**:
  - генерация криптостойких случайных байтов (**Entropy + CTR_DRBG**)
  - пример блочного шифрования/дешифрования (**2-key 3DES + ECB + PKCS#5 padding**) — только для демонстрации.

> Важно: 3DES и режим ECB считаются устаревшими/небезопасными для реальных систем. Здесь это сделано как простой пример вызова криптофункций mbedTLS из JNI.

---

## Требования
- Android Studio + Android SDK
- Android NDK (через SDK Manager)
- CMake (через SDK Manager)
- Сборка запускается под выбранный ABI (обычно `arm64-v8a`)

---

## Внешние зависимости (3rd-party)

В проекте используются сторонние репозитории, которые **не хранятся** в этом репозитории (их нужно скачать отдельно в `libs/`):

- **mbedTLS** (фиксируем версию): `v3.6.5`
  - при сборке могут использоваться 3rdparty компоненты `everest` и `p256-m` (в зависимости от конфигурации сборки)
- **spdlog**

### Ожидаемая структура папок
```
fclient/
  app/
  libs/
    mbedtls/
      mbedtls/            # git clone mbedtls сюда
      build/arm64-v8a/    # результат сборки (libmbedtls.so/libmbedcrypto.so и др.)
      compile.sh          # ваш скрипт сборки (если используете)
    spdlog/
      spdlog/             # git clone spdlog сюда
      build/arm64-v8a/    # результат сборки (libspdlog.a)
      compile.sh          # ваш скрипт сборки (если используете)
```

---

## Как скачать зависимости (один раз)

Из корня проекта:

```bash
cd libs/spdlog

# spdlog
git clone https://github.com/gabime/spdlog.git libs/spdlog/spdlog

# mbedtls (важно: версия 3.6.5)
git clone https://github.com/Mbed-TLS/mbedtls.git libs/mbedtls/mbedtls
cd libs/mbedtls/mbedtls
git checkout v3.6.5
cd ../../../
```

---

## Как собрать зависимости

Далее соберите зависимости под нужный ABI (например, `arm64-v8a`) вашим способом.

```bash
cd libs/spdlog/build/
./compile.sh
cd ../../

cd libs/mbedtls/build/
./compile.sh
cd ../../
```

Проверьте, что появились артефакты (пример для `arm64-v8a`):
- `libs/spdlog/build/arm64-v8a/libspdlog.a`
- `libs/mbedtls/build/arm64-v8a/library/libmbedtls.so`
- `libs/mbedtls/build/arm64-v8a/library/libmbedcrypto.so`
- (опционально) `libs/mbedtls/build/arm64-v8a/3rdparty/.../*.so` (everest/p256m), если ваша сборка делает их shared

---

## Сборка и запуск приложения

1. Открыть проект в Android Studio
2. Gradle Sync
3. Run на устройстве/эмуляторе нужного ABI

---

## Примечание по безопасности
Код предназначен для обучения (JNI + NDK + вызовы mbedTLS). Для реального шифрования вместо 3DES/ECB используйте современные режимы (например AES-GCM или ChaCha20-Poly1305) и проверяйте паддинг/ошибки строго по правилам безопасной криптографии.
