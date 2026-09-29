import java.util.Properties

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kotlin.compose)
    alias(libs.plugins.kotlin.serialization)
}

// Connection settings live in android/secrets.properties (git-ignored).
// Copy secrets.properties.example and fill in your own values.
val secrets = Properties().apply {
    val file = rootProject.file("secrets.properties")
    if (file.exists()) file.inputStream().use { load(it) }
}

fun secret(key: String, default: String = ""): String =
    "\"" + secrets.getProperty(key, default).replace("\\", "\\\\").replace("\"", "\\\"") + "\""

android {
    namespace = "com.smartvalve.app"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.smartvalve.app"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"

        buildConfigField("String", "MQTT_HOST", secret("mqtt.host"))
        buildConfigField("int", "MQTT_PORT", secrets.getProperty("mqtt.port", "8883"))
        buildConfigField("String", "MQTT_USERNAME", secret("mqtt.username"))
        buildConfigField("String", "MQTT_PASSWORD", secret("mqtt.password"))
        buildConfigField("String", "TOPIC_PRESSURE", secret("mqtt.topic.pressure", "smartvalve/pressure"))
        buildConfigField("String", "TOPIC_STATUS", secret("mqtt.topic.status", "smartvalve/status"))
        buildConfigField("String", "BACKEND_URL", secret("backend.url"))
        buildConfigField("String", "BACKEND_API_KEY", secret("backend.apiKey"))
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
    buildFeatures {
        compose = true
        buildConfig = true
    }
    // The HiveMQ client (Netty) ships duplicate META-INF files that break packaging.
    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
            excludes += "META-INF/INDEX.LIST"
            excludes += "META-INF/io.netty.versions.properties"
            excludes += "META-INF/DEPENDENCIES"
        }
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    // Provides the Theme.Material3.* XML theme used by the manifest at launch.
    implementation(libs.material)

    // --- Jetpack Compose ---
    val composeBom = platform(libs.androidx.compose.bom)
    implementation(composeBom)
    implementation(libs.androidx.ui)
    implementation(libs.androidx.ui.graphics)
    implementation(libs.androidx.ui.tooling.preview)
    implementation(libs.androidx.material3)
    implementation(libs.androidx.activity.compose)
    implementation(libs.androidx.lifecycle.viewmodel.compose)
    implementation(libs.androidx.lifecycle.runtime.compose)
    debugImplementation(libs.androidx.ui.tooling)

    // --- MQTT (live pressure) ---
    implementation(libs.hivemq.mqtt.client)

    // --- HTTP (valve control through the backend) ---
    implementation(libs.retrofit)
    implementation(libs.retrofit.kotlinx.serialization)
    implementation(libs.okhttp)
    implementation(libs.kotlinx.serialization.json)
    implementation(libs.kotlinx.coroutines.android)

    // --- Unit tests ---
    testImplementation(libs.junit)
    testImplementation(libs.mockk)
    testImplementation(libs.kotlinx.coroutines.test)
    testImplementation(libs.truth)

}
