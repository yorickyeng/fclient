package ru.iu3.fclient

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts.StartActivityForResult
import androidx.appcompat.app.AppCompatActivity
import org.apache.commons.codec.binary.Hex
import ru.iu3.fclient.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var pinpadLauncher: ActivityResultLauncher<Intent>

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        binding.sampleButton.text = stringFromJNI()

        check(initRng() == 0)
        registerPinpadLauncher()
    }

    override fun onDestroy() {
        super.onDestroy()
        freeRng()
    }

    fun onButtonClick(v: View) {
        val text = getDecryptedText()
        Toast.makeText(this, text, Toast.LENGTH_SHORT).show()

        val intent = Intent(this, PinpadActivity::class.java)
        pinpadLauncher.launch(intent)
    }

    private fun registerPinpadLauncher() {
        pinpadLauncher = registerForActivityResult(
            StartActivityForResult()
        ) { result ->
            if (result.resultCode == RESULT_OK) {
                val pin = result.data?.getStringExtra("pin") ?: "Pin is null"
                Toast.makeText(this, pin, Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun getDecryptedText(): String {
        val key = stringToHex("0123456789ABCDEF0123456789ABCDE0") ?: return ""
        val data = stringToHex("000000000000000102") ?: return ""

        val enc = encrypt(key, data)
        val dec = decrypt(key, enc)

        return Hex.encodeHexString(dec)
    }

    /**
     * A native method that is implemented by the 'fclient' native library,
     * which is packaged with this application.
     */
    private external fun stringFromJNI(): String

    companion object {
        private fun stringToHex(s: String): ByteArray? =
            runCatching { Hex.decodeHex(s.toCharArray()) }.getOrNull()

        init { System.loadLibrary("fclient") }

        @JvmStatic external fun initRng(): Int
        @JvmStatic external fun freeRng()
        @JvmStatic external fun randomBytes(len: Int): ByteArray
        @JvmStatic external fun encrypt(key: ByteArray, data: ByteArray): ByteArray
        @JvmStatic external fun decrypt(key: ByteArray, data: ByteArray): ByteArray
    }
}