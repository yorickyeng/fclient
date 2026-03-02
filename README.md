# fclient
Учебное Android-приложение, демонстрирующее архитектуру, где:
- Kotlin/Android UI вызывает C++ через **JNI** (NDK);
- C++ запускает **фоновую транзакцию** (std::thread) и делает **callback в Kotlin**:
  - запросить ввод PIN (`enterPin`)
  - вернуть результат транзакции (`transactionResult`)
- ввод PIN выполняется в отдельной Activity (**PinpadActivity**) и передаётся обратно в C++ через JNI (`providePin`);
- из C++ ведётся логирование в **Logcat** через **spdlog** (`android_sink`);
- используется **mbedTLS** для:
  - генерации криптостойких случайных байтов (Entropy + CTR_DRBG),
  - демонстрационного шифрования/дешифрования (2-key 3DES + ECB + PKCS#5 padding).

> Важно: 3DES и режим ECB небезопасны для реальных систем и используются здесь только как простой пример интеграции mbedTLS через JNI.

---

## Основной сценарий (что делает приложение)
1. Пользователь нажимает кнопку “транзакции” в `MainActivity`.
2. Kotlin вызывает JNI-метод `transaction(trd: ByteArray)`.
3. В C++ создаются `GlobalRef` на `MainActivity` и `trd`, запускается фоновый поток.
4. Фоновый поток:
   - парсит сумму из `TRD`,
   - вызывает callback `enterPin(attempts, amount)` (C++ → Kotlin) для показа PIN-pad,
   - ждёт PIN через `condition_variable`.
5. `PinpadActivity` возвращает PIN в `MainActivity`, после чего Kotlin вызывает `providePin(pin)` (Kotlin → C++).
6. C++ проверяет PIN, повторяет до 3 попыток, затем вызывает `transactionResult(ok)` (C++ → Kotlin), UI показывает Toast.
7. (Дополнительно) после 3 неверных попыток ввод временно блокируется (пример анти-брутфорса).

---

## Модули/файлы (где что находится)
- **Kotlin / UI**
  - `MainActivity`:
    - реализует интерфейс транзакционных событий (enterPin / transactionResult)
    - запускает `PinpadActivity` через `ActivityResultLauncher`
    - отправляет введённый PIN в native через `providePin`
  - `PinpadActivity`:
    - UI ввода PIN + отображение суммы и числа попыток
    - перемешивание клавиш (использует `randomBytes` из native)

- **C++ / NDK**
  - `JNI_OnLoad`:
    - сохраняет `JavaVM*` для последующего attach/detach фоновых потоков
  - `transaction`:
    - создаёт глобальные ссылки
    - запускает `std::thread`
    - вызывает Java-методы через `GetMethodID` + `CallVoidMethod`
    - синхронизируется с UI через `providePin` и `condition_variable`
  - `providePin`:
    - получает PIN из Kotlin и будит поток транзакции
  - `spdlog(...)`:
    - пишет сообщения в Logcat
  - `initRng/freeRng/randomBytes`, `encrypt/decrypt`:
    - демонстрация mbedTLS и вызовов из Kotlin

---

## Требования
- Android Studio + Android SDK
- Android NDK (через SDK Manager)
- CMake (через SDK Manager)
- Устройство/эмулятор с ABI, под который собраны зависимости (обычно `arm64-v8a`)

---

## Внешние зависимости (3rd-party)
В проекте используются внешние библиотеки, которые размещаются в `libs/`:
- **mbedTLS**: `v3.6.5`
- **spdlog**

### Ожидаемая структура папок
```
fclient/
  app/
  libs/
    mbedtls/
      mbedtls/            # исходники mbedtls
      build/arm64-v8a/    # результаты сборки
      compile.sh          # (опционально) ваш скрипт сборки
    spdlog/
      spdlog/             # исходники spdlog
      build/arm64-v8a/    # результаты сборки
      compile.sh          # (опционально) ваш скрипт сборки
```

---

## Как скачать зависимости (один раз)
Из корня проекта:
```bash
# spdlog
git clone https://github.com/gabime/spdlog.git libs/spdlog/spdlog

# mbedtls (версия 3.6.5)
git clone https://github.com/Mbed-TLS/mbedtls.git libs/mbedtls/mbedtls
cd libs/mbedtls/mbedtls
git checkout v3.6.5
cd ../../../
```

---

## Как собрать зависимости
Соберите под нужный ABI (пример: `arm64-v8a`) вашим способом/скриптами:
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

---

## Сборка и запуск приложения
1. Открыть проект в Android Studio
2. Gradle Sync
3. Run на устройстве/эмуляторе подходящего ABI

---

## Примечания по JNI/потокам (важное)
- `JNIEnv*` нельзя переносить между потоками — в фоне используется `AttachCurrentThread/DetachCurrentThread`.
- Объекты `thiz` и `trd` передаются в фоновые потоки через `NewGlobalRef`, затем освобождаются `DeleteGlobalRef`.
- Вызовы UI выполняются в `runOnUiThread` (Kotlin), чтобы не нарушать правила Android.

---

## Примечание по безопасности
Проект учебный: демонстрация JNI + архитектуры обратных вызовов C++ ↔ Kotlin.
Для реальных платёжных/криптосистем используйте современные алгоритмы и режимы (например, AES-GCM или ChaCha20-Poly1305), корректную обработку ошибок/паддинга и защищённую работу с PIN (zeroization, защита памяти, аппаратные модули, PCI DSS и т.д.).
```
