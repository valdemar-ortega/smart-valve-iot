package com.smartvalve.app.data

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

/** One reading published by the ESP32 on the pressure topic (see docs/protocol.md). */
@Serializable
data class PressureReading(
    val psi: Double = 0.0,
    val voltage: Double = 0.0,
    val raw: Int = 0,
    val fault: Boolean = false,
    @SerialName("uptime_ms") val uptimeMs: Long = 0L,
)
