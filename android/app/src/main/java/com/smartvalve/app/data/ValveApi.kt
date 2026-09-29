package com.smartvalve.app.data

import kotlinx.serialization.Serializable
import retrofit2.http.GET
import retrofit2.http.Header
import retrofit2.http.POST

@Serializable
data class ValveStatus(val open: Boolean? = null)

/** Tuya's command reply, forwarded as-is by the backend. */
@Serializable
data class CommandResponse(
    val success: Boolean = false,
    val code: Int? = null,
    val msg: String? = null,
)

/** REST API exposed by backend/server.js. */
interface ValveApi {
    @POST("valve/open")
    suspend fun open(@Header("x-api-key") key: String): CommandResponse

    @POST("valve/close")
    suspend fun close(@Header("x-api-key") key: String): CommandResponse

    @GET("valve/status")
    suspend fun status(@Header("x-api-key") key: String): ValveStatus
}
