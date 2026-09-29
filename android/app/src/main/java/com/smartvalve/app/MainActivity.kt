package com.smartvalve.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.ui.Modifier
import androidx.lifecycle.viewmodel.compose.viewModel
import com.smartvalve.app.data.MqttSettings
import com.smartvalve.app.data.PressureRepository
import com.smartvalve.app.data.ValveApi
import com.smartvalve.app.data.ValveRepository
import com.smartvalve.app.ui.NotConfiguredScreen
import com.smartvalve.app.ui.ValveScreen
import com.smartvalve.app.ui.ValveViewModel
import kotlinx.serialization.json.Json
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import retrofit2.Retrofit
import retrofit2.converter.kotlinx.serialization.asConverterFactory

class MainActivity : ComponentActivity() {

    /** True once android/secrets.properties has been filled in. */
    private val isConfigured: Boolean
        get() = BuildConfig.MQTT_HOST.isNotBlank() && BuildConfig.BACKEND_URL.startsWith("http")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            MaterialTheme(colorScheme = if (isSystemInDarkTheme()) darkColorScheme() else lightColorScheme()) {
                Scaffold(modifier = Modifier.fillMaxSize()) { padding ->
                    if (isConfigured) {
                        val vm: ValveViewModel = viewModel(factory = SimpleFactory { createViewModel() })
                        ValveScreen(vm, Modifier.padding(padding))
                    } else {
                        NotConfiguredScreen(Modifier.padding(padding))
                    }
                }
            }
        }
    }

    private fun createViewModel(): ValveViewModel {
        val pressureRepo = PressureRepository(
            MqttSettings(
                host = BuildConfig.MQTT_HOST,
                port = BuildConfig.MQTT_PORT,
                username = BuildConfig.MQTT_USERNAME,
                password = BuildConfig.MQTT_PASSWORD,
                pressureTopic = BuildConfig.TOPIC_PRESSURE,
                statusTopic = BuildConfig.TOPIC_STATUS,
            ),
        )

        val json = Json { ignoreUnknownKeys = true }
        val api = Retrofit.Builder()
            .baseUrl(BuildConfig.BACKEND_URL)
            .client(OkHttpClient())
            .addConverterFactory(json.asConverterFactory("application/json".toMediaType()))
            .build()
            .create(ValveApi::class.java)

        return ValveViewModel(pressureRepo, ValveRepository(api, BuildConfig.BACKEND_API_KEY))
    }
}
