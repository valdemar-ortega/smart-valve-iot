package com.smartvalve.app.data

import com.google.common.truth.Truth.assertThat
import kotlinx.serialization.json.Json
import org.junit.Test

/** Guards the JSON contract between firmware/main/mqtt_link.c and the app. */
class PressureReadingTest {

    private val json = Json { ignoreUnknownKeys = true }

    @Test
    fun `parses the exact payload the firmware publishes`() {
        val payload = """{"psi":87.0,"voltage":1.659,"raw":2051,"fault":false,"uptime_ms":123456}"""

        val r = json.decodeFromString<PressureReading>(payload)

        assertThat(r.psi).isEqualTo(87.0)
        assertThat(r.voltage).isEqualTo(1.659)
        assertThat(r.raw).isEqualTo(2051)
        assertThat(r.fault).isFalse()
        assertThat(r.uptimeMs).isEqualTo(123456L)
    }

    @Test
    fun `a fault payload keeps the flag`() {
        val r = json.decodeFromString<PressureReading>(
            """{"psi":0.0,"voltage":0.020,"raw":25,"fault":true,"uptime_ms":1}""",
        )

        assertThat(r.fault).isTrue()
        assertThat(r.psi).isEqualTo(0.0)
    }

    @Test
    fun `unknown fields from newer firmware are ignored`() {
        val r = json.decodeFromString<PressureReading>("""{"psi":1.5,"temperature":25}""")

        assertThat(r.psi).isEqualTo(1.5)
    }
}
