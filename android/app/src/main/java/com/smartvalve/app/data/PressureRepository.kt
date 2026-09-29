package com.smartvalve.app.data

import com.hivemq.client.mqtt.MqttGlobalPublishFilter
import com.hivemq.client.mqtt.datatypes.MqttQos
import com.hivemq.client.mqtt.mqtt5.Mqtt5AsyncClient
import com.hivemq.client.mqtt.mqtt5.Mqtt5Client
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.serialization.json.Json
import java.util.UUID

/** Events coming from the ESP32 through the broker. */
sealed interface MqttUpdate {
    /** A reading from the pressure topic. */
    data class Pressure(val reading: PressureReading) : MqttUpdate

    /** Board status from the status topic: "online", or "offline" via its Last Will. */
    data class BoardStatus(val online: Boolean) : MqttUpdate
}

/** Broker connection settings. */
data class MqttSettings(
    val host: String,
    val port: Int,
    val username: String,
    val password: String,
    val pressureTopic: String,
    val statusTopic: String,
)

/**
 * Live pressure and board status over MQTT/TLS.
 * One connection, two subscriptions, a single [Flow] of [MqttUpdate]s.
 */
open class PressureRepository(
    private val settings: MqttSettings,
    private val json: Json = Json { ignoreUnknownKeys = true },
) {
    private val client: Mqtt5AsyncClient by lazy {
        Mqtt5Client.builder()
            .identifier("android-" + UUID.randomUUID())
            .serverHost(settings.host)
            .serverPort(settings.port)
            .sslWithDefaultConfig()
            .automaticReconnectWithDefaultConfig()
            .buildAsync()
    }

    open fun updates(): Flow<MqttUpdate> = callbackFlow {
        client.publishes(MqttGlobalPublishFilter.ALL) { publish ->
            val payload = String(publish.payloadAsBytes)
            when (publish.topic.toString()) {
                settings.pressureTopic ->
                    runCatching { json.decodeFromString<PressureReading>(payload) }
                        .onSuccess { trySend(MqttUpdate.Pressure(it)) }

                settings.statusTopic ->
                    trySend(MqttUpdate.BoardStatus(payload.trim() == "online"))
            }
        }

        client.connectWith()
            .simpleAuth()
            .username(settings.username)
            .password(settings.password.toByteArray())
            .applySimpleAuth()
            .send()
            .whenComplete { _, err ->
                if (err != null) {
                    close(err)
                    return@whenComplete
                }
                listOf(settings.pressureTopic, settings.statusTopic).forEach { topic ->
                    client.subscribeWith().topicFilter(topic).qos(MqttQos.AT_LEAST_ONCE).send()
                }
            }

        awaitClose { client.disconnect() }
    }
}
