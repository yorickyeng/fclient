package ru.iu3.fclient

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import ru.iu3.fclient.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        binding.sampleText.text = stringFromJNI()

        val rng = initRng()

        val key = randomBytes(16)
        val data = randomBytes(100)
        val enc = encrypt(key, data)
        val dec = decrypt(key, enc)

        check(data.contentEquals(dec)) { "decrypt(encrypt(data)) != data" }
    }

    /**
     * A native method that is implemented by the 'fclient' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'fclient' library on application startup.
        init { System.loadLibrary("fclient") }

        @JvmStatic external fun initRng(): Int
        @JvmStatic external fun randomBytes(len: Int): ByteArray
        @JvmStatic external fun encrypt(key: ByteArray, data: ByteArray): ByteArray
        @JvmStatic external fun decrypt(key: ByteArray, data: ByteArray): ByteArray
    }
}