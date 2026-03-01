package ru.iu3.fclient

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity

const val MAX_KEYS = 10

class PinpadActivity : AppCompatActivity() {

    private lateinit var textViewPin: TextView
    private var pin: String = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_pinpad)

        textViewPin = findViewById(R.id.txtPin)

        shuffleKeys()

        val okButton: Button = findViewById(R.id.btnOK)
        val resetButton: Button = findViewById(R.id.btnReset)

        okButton.setOnClickListener {
            val intent = Intent().apply { putExtra("pin", pin) }
            setResult(RESULT_OK, intent)
            finish()
        }
        resetButton.setOnClickListener {
            pin = ""
            textViewPin.text = ""
        }
    }

    fun keyClick(v: View) {
        val key = (v as TextView).getText().toString()
        if (pin.length < 4 && key.length == 1 && key[0].isDigit()) {
            pin += key
            textViewPin.text = "*".repeat(pin.length)
        }
    }

    private fun shuffleKeys() {
        val randomBytes = MainActivity.randomBytes(MAX_KEYS)
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

        for (i in keys.lastIndex downTo 1) {
            val j = (randomBytes[i].toInt() and 0xFF) % (i + 1)
            keys[i].text = keys[j].text.also { keys[j].text = keys[i].text }
        }
    }
}