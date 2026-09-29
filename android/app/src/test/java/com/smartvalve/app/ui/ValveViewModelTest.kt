package com.smartvalve.app.ui

import com.google.common.truth.Truth.assertThat
import com.smartvalve.app.data.MqttUpdate
import com.smartvalve.app.data.PressureReading
import com.smartvalve.app.data.PressureRepository
import com.smartvalve.app.data.ValveCommandException
import com.smartvalve.app.data.ValveRepository
import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.every
import io.mockk.mockk
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOf
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.advanceUntilIdle
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Before
import org.junit.Test

@OptIn(ExperimentalCoroutinesApi::class)
class ValveViewModelTest {

    private val dispatcher = StandardTestDispatcher()
    private val pressureRepo = mockk<PressureRepository>()
    private val valveRepo = mockk<ValveRepository>(relaxed = true)

    @Before
    fun setUp() = Dispatchers.setMain(dispatcher)

    @After
    fun tearDown() = Dispatchers.resetMain()

    @Test
    fun `a reading updates pressure and marks the board online`() = runTest(dispatcher) {
        every { pressureRepo.updates() } returns flowOf(MqttUpdate.Pressure(PressureReading(psi = 87.0)))
        coEvery { valveRepo.isOpen() } returns false

        val vm = ValveViewModel(pressureRepo, valveRepo)
        advanceUntilIdle()

        val s = vm.state.value
        assertThat(s.pressure?.psi).isEqualTo(87.0)
        assertThat(s.boardOnline).isTrue()
        assertThat(s.sensorOk).isTrue()
        assertThat(s.valveOpen).isFalse()
    }

    @Test
    fun `a faulted reading is not reported as sensor ok`() = runTest(dispatcher) {
        every { pressureRepo.updates() } returns flowOf(MqttUpdate.Pressure(PressureReading(fault = true)))

        val vm = ValveViewModel(pressureRepo, valveRepo)
        advanceUntilIdle()

        assertThat(vm.state.value.boardOnline).isTrue()
        assertThat(vm.state.value.sensorOk).isFalse()
    }

    @Test
    fun `last will offline marks the board offline`() = runTest(dispatcher) {
        every { pressureRepo.updates() } returns flowOf(
            MqttUpdate.Pressure(PressureReading(psi = 10.0)),
            MqttUpdate.BoardStatus(online = false),
        )

        val vm = ValveViewModel(pressureRepo, valveRepo)
        advanceUntilIdle()

        assertThat(vm.state.value.boardOnline).isFalse()
        assertThat(vm.state.value.sensorOk).isFalse()
    }

    @Test
    fun `broker error is shown and board is offline`() = runTest(dispatcher) {
        every { pressureRepo.updates() } returns flow { throw IllegalStateException("bad credentials") }

        val vm = ValveViewModel(pressureRepo, valveRepo)
        advanceUntilIdle()

        assertThat(vm.state.value.error).isEqualTo("bad credentials")
        assertThat(vm.state.value.boardOnline).isFalse()
    }

    @Test
    fun `open calls the backend and refreshes the valve state`() = runTest(dispatcher) {
        every { pressureRepo.updates() } returns flowOf()
        coEvery { valveRepo.isOpen() } returns true

        val vm = ValveViewModel(pressureRepo, valveRepo)
        vm.openValve()
        advanceUntilIdle()

        coVerify(exactly = 1) { valveRepo.open() }
        assertThat(vm.state.value.valveOpen).isTrue()
        assertThat(vm.state.value.busy).isFalse()
    }

    @Test
    fun `a refused command surfaces the cloud message`() = runTest(dispatcher) {
        every { pressureRepo.updates() } returns flowOf()
        coEvery { valveRepo.close() } throws ValveCommandException("device is offline")

        val vm = ValveViewModel(pressureRepo, valveRepo)
        vm.closeValve()
        advanceUntilIdle()

        assertThat(vm.state.value.error).isEqualTo("device is offline")
        assertThat(vm.state.value.busy).isFalse()
    }

    @Test
    fun `dismissing the error clears it`() = runTest(dispatcher) {
        every { pressureRepo.updates() } returns flow { throw IllegalStateException("x") }

        val vm = ValveViewModel(pressureRepo, valveRepo)
        advanceUntilIdle()
        vm.dismissError()

        assertThat(vm.state.value.error).isNull()
    }
}
