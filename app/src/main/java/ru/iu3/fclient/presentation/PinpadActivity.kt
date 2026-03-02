package ru.iu3.fclient.presentation

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import ru.iu3.fclient.R
import ru.iu3.fclient.databinding.ActivityPinpadBinding
import java.text.DecimalFormat

/**
 * @author ay.sultanov
 */
class PinpadActivity : AppCompatActivity() {

    private lateinit var binding: ActivityPinpadBinding
    private var pin: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        bindContent()
    }

    fun onKeyClick(v: View) {
        val key = (v as TextView).getText().toString()
        if (pin.length < 4 && key.singleOrNull()?.isDigit() == true) {
            pin += key
            binding.txtPin.setText("*".repeat(pin.length))
        }
    }

    private fun bindContent() {
        binding = ActivityPinpadBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setAmountAndAttempts()
        setOnClickListeners()
        shuffleKeys()
    }

    private fun setAmountAndAttempts() = with(binding) {
        val amount = intent.getStringExtra("amount")?.toLongOrNull()
        val attempts = intent.getIntExtra("attempts", 0)

        txtAmount.text = amount?.let { formatAmount(it) } ?: getString(R.string.amount_unknown)
        txtAttempts.text = formatAttempts(attempts)
    }

    private fun setOnClickListeners() = with(binding) {
        btnOK.setOnClickListener {
            setResult(RESULT_OK, Intent().putExtra("pin", pin))
            finish()
        }

        btnReset.setOnClickListener {
            pin = ""
            txtPin.setText("")
        }
    }

    private fun shuffleKeys() {
        val keys: Array<Button> = arrayOf(
            findViewById(R.id.btnKey0),
            findViewById(R.id.btnKey1),
            findViewById(R.id.btnKey2),
            findViewById(R.id.btnKey3),
            findViewById(R.id.btnKey4),
            findViewById(R.id.btnKey5),
            findViewById(R.id.btnKey6),
            findViewById(R.id.btnKey7),
            findViewById(R.id.btnKey8),
            findViewById(R.id.btnKey9),
        )
        val randomBytes = MainActivity.randomBytes(keys.size)

        for (i in keys.lastIndex downTo 1) {
            val j = (randomBytes[i].toInt() and 0xFF) % (i + 1)
            keys[i].text = keys[j].text.also { keys[j].text = keys[i].text }
        }
    }

    private fun formatAmount(amount: Long): String {
        val df = DecimalFormat(getString(R.string.decimal_format))
        return df.format(amount)
    }

    private fun formatAttempts(attempts: Int): String =
        when (attempts) {
            2 -> getString(R.string.pinpad_two_attempts_left)
            1 -> getString(R.string.pinpad_one_attempt_left)
            else -> getString(R.string.pinpad_attempts, attempts)
        }
}