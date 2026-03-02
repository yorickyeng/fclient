package ru.iu3.fclient.domain

/**
 * @author ay.sultanov
 */
interface TransactionEvent {

    fun enterPin(attempts: Int, amount: String)
    fun transactionResult(result: Boolean)
}