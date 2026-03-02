package ru.iu3.fclient.presentation

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts.StartActivityForResult
import androidx.appcompat.app.AppCompatActivity
import org.apache.commons.codec.binary.Hex
import ru.iu3.fclient.R
import ru.iu3.fclient.databinding.ActivityMainBinding
import ru.iu3.fclient.domain.TransactionEvent

/**
 * @author ay.sultanov
 */
class MainActivity : AppCompatActivity(), TransactionEvent {

    private lateinit var binding: ActivityMainBinding
    private lateinit var pinpadLauncher: ActivityResultLauncher<Intent>

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        bindContent()
        initRng()
        registerPinpadLauncher()
    }

    override fun onDestroy() {
        super.onDestroy()
        freeRng()
    }

    override fun enterPin(attempts: Int, amount: String) {
        runOnUiThread {
            val intent = Intent(this, PinpadActivity::class.java).apply {
                putExtra("attempts", attempts)
                putExtra("amount", amount)
            }
            pinpadLauncher.launch(intent)
        }
    }

    override fun transactionResult(result: Boolean) {
        val text = if (result) "ok" else "wrong PIN"
        runOnUiThread {
            Toast.makeText(this, text, Toast.LENGTH_SHORT).show()
        }
    }

    fun onButtonClick(v: View) {
        when (v.id) {
            R.id.pinpad_button -> {
                val trd = stringToHex("9F0206000000000100") ?: return
                transaction(trd)
            }

            R.id.encrypt_button -> cipherTest()
        }
    }

    private fun bindContent() {
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
    }

    private fun registerPinpadLauncher() {
        pinpadLauncher = registerForActivityResult(
            StartActivityForResult()
        ) { result ->
            if (result.resultCode == RESULT_OK) {
                val pin = result.data?.getStringExtra("pin") ?: return@registerForActivityResult
                providePin(pin) // send pin to native
            }
        }
    }

    private fun cipherTest() {
        val key = randomBytes(16)
        val data = stringToHex("69206c6f766520796f75") ?: return

        val encrypted = encrypt(key, data)
        val decrypted = decrypt(key, encrypted)

        val text =
            "${data.toString(Charsets.UTF_8)} -> " +
            "${encrypted.toString(Charsets.UTF_8)} -> " +
            decrypted.toString(Charsets.UTF_8)

        Toast.makeText(this, text, Toast.LENGTH_SHORT).show()
        spdlog("my awesome spdlog TAG", text)
    }

    private external fun transaction(trd: ByteArray): Boolean
    private external fun providePin(pin: String)

    companion object {
        private fun stringToHex(s: String): ByteArray? =
            runCatching { Hex.decodeHex(s.toCharArray()) }.getOrNull()

        init {
            System.loadLibrary("fclient")
        }

        @JvmStatic external fun randomBytes(len: Int): ByteArray
        @JvmStatic external fun spdlog(tag: String, message: String)
        @JvmStatic private external fun initRng(): Int
        @JvmStatic private external fun freeRng()
        @JvmStatic private external fun encrypt(key: ByteArray, data: ByteArray): ByteArray
        @JvmStatic private external fun decrypt(key: ByteArray, data: ByteArray): ByteArray
    }
}