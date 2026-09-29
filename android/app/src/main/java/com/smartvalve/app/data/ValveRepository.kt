package com.smartvalve.app.data

/** Thrown when the cloud accepts the request but refuses the command. */
class ValveCommandException(message: String) : Exception(message)

/** Opens, closes and queries the valve through the backend. */
open class ValveRepository(
    private val api: ValveApi,
    private val apiKey: String,
) {
    open suspend fun open() = api.open(apiKey).requireSuccess()

    open suspend fun close() = api.close(apiKey).requireSuccess()

    open suspend fun isOpen(): Boolean? = api.status(apiKey).open

    private fun CommandResponse.requireSuccess() {
        if (!success) {
            throw ValveCommandException(msg?.takeIf { it.isNotBlank() } ?: "cloud error ${code ?: "?"}")
        }
    }
}
