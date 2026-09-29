package com.smartvalve.app.ui

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.smartvalve.app.data.MqttUpdate
import com.smartvalve.app.data.PressureReading
import com.smartvalve.app.data.PressureRepository
import com.smartvalve.app.data.ValveRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

data class UiState(
    /** Last reading, or null until the first one arrives. */
    val pressure: PressureReading? = null,
    /** True while the ESP32 is connected to the broker. */
    val boardOnline: Boolean = false,
    /** null = unknown (backend not reached yet). */
    val valveOpen: Boolean? = null,
    /** A valve command is in flight. */
    val busy: Boolean = false,
    val error: String? = null,
) {
    /** Board online and the last reading is not a wiring fault. */
    val sensorOk: Boolean get() = boardOnline && pressure?.fault == false
}

class ValveViewModel(
    private val pressureRepo: PressureRepository,
    private val valveRepo: ValveRepository,
) : ViewModel() {

    private val _state = MutableStateFlow(UiState())
    val state: StateFlow<UiState> = _state.asStateFlow()

    init {
        observeBoard()
        refreshValve()
    }

    private fun observeBoard() = viewModelScope.launch {
        pressureRepo.updates()
            .catch { e -> _state.update { it.copy(error = e.message, boardOnline = false) } }
            .collect { update ->
                when (update) {
                    // A fresh reading implies the board is online, even if the
                    // retained status message has not arrived yet.
                    is MqttUpdate.Pressure -> _state.update {
                        it.copy(pressure = update.reading, boardOnline = true)
                    }
                    is MqttUpdate.BoardStatus -> _state.update { it.copy(boardOnline = update.online) }
                }
            }
    }

    fun refreshValve() = viewModelScope.launch {
        runCatching { valveRepo.isOpen() }
            .onSuccess { open -> _state.update { it.copy(valveOpen = open) } }
            .onFailure { e -> _state.update { it.copy(error = e.message) } }
    }

    fun openValve() = runCommand { valveRepo.open() }

    fun closeValve() = runCommand { valveRepo.close() }

    fun dismissError() = _state.update { it.copy(error = null) }

    private fun runCommand(command: suspend () -> Unit) = viewModelScope.launch {
        if (_state.value.busy) return@launch
        _state.update { it.copy(busy = true, error = null) }
        runCatching { command() }
            .onFailure { e -> _state.update { it.copy(error = e.message) } }
        _state.update { it.copy(busy = false) }
        refreshValve()
    }
}
