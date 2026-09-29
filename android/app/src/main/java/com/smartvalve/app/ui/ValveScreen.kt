package com.smartvalve.app.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.AssistChip
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.smartvalve.app.R
import java.util.Locale

/** Full-scale pressure of the sensor, used by the gauge. */
private const val GAUGE_MAX_PSI = 174f

/** Above this fraction of full scale the gauge turns to the warning color. */
private const val GAUGE_WARNING_FRACTION = 0.85f

@Composable
fun ValveScreen(vm: ValveViewModel, modifier: Modifier = Modifier) {
    val s by vm.state.collectAsStateWithLifecycle()

    Column(
        modifier.fillMaxSize().padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(20.dp),
    ) {
        Text(stringResource(R.string.app_name), fontSize = 26.sp, fontWeight = FontWeight.Bold)

        AssistChip(
            onClick = {},
            label = {
                Text(
                    stringResource(
                        when {
                            !s.boardOnline -> R.string.board_offline
                            s.sensorOk -> R.string.sensor_ok
                            else -> R.string.sensor_fault
                        },
                    ),
                )
            },
        )

        val psi = s.pressure?.psi?.toFloat() ?: 0f
        PressureGauge(psi = psi, max = GAUGE_MAX_PSI)
        Text(
            if (s.pressure == null) "-- psi" else String.format(Locale.US, "%.1f psi", psi),
            fontSize = 40.sp,
            fontWeight = FontWeight.Bold,
        )
        if (s.pressure?.fault == true) {
            Text(
                stringResource(R.string.sensor_fault_hint),
                color = MaterialTheme.colorScheme.error,
                textAlign = TextAlign.Center,
            )
        }

        Text(
            stringResource(
                when (s.valveOpen) {
                    true -> R.string.valve_open
                    false -> R.string.valve_closed
                    null -> R.string.valve_unknown
                },
            ),
            fontSize = 20.sp,
        )

        Row(horizontalArrangement = Arrangement.spacedBy(16.dp)) {
            Button(onClick = { vm.openValve() }, enabled = !s.busy) {
                Text(stringResource(R.string.action_open))
            }
            OutlinedButton(onClick = { vm.closeValve() }, enabled = !s.busy) {
                Text(stringResource(R.string.action_close))
            }
        }

        if (s.busy) CircularProgressIndicator()

        s.error?.let { message ->
            Text(
                stringResource(R.string.error_prefix, message),
                color = MaterialTheme.colorScheme.error,
                textAlign = TextAlign.Center,
            )
            TextButton(onClick = { vm.dismissError() }) { Text(stringResource(R.string.action_dismiss)) }
        }
    }
}

/** Shown instead of the main screen when android/secrets.properties is missing. */
@Composable
fun NotConfiguredScreen(modifier: Modifier = Modifier) {
    Column(
        modifier.fillMaxSize().padding(32.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Text(stringResource(R.string.not_configured_title), fontSize = 22.sp, fontWeight = FontWeight.Bold)
        Text(
            stringResource(R.string.not_configured_body),
            modifier = Modifier.padding(top = 12.dp),
            textAlign = TextAlign.Center,
        )
    }
}

/** 270° arc gauge. */
@Composable
fun PressureGauge(psi: Float, max: Float) {
    val fraction = (psi / max).coerceIn(0f, 1f)
    val track = MaterialTheme.colorScheme.surfaceVariant
    val normal = MaterialTheme.colorScheme.primary
    val warning = MaterialTheme.colorScheme.error

    Canvas(Modifier.size(200.dp)) {
        val strokePx = 28f
        val stroke = Stroke(width = strokePx, cap = StrokeCap.Round)
        val arcSize = Size(size.width - strokePx, size.height - strokePx)
        val topLeft = Offset(strokePx / 2, strokePx / 2)
        drawArc(track, 135f, 270f, false, topLeft, arcSize, style = stroke)
        val color = if (fraction > GAUGE_WARNING_FRACTION) warning else normal
        drawArc(color, 135f, 270f * fraction, false, topLeft, arcSize, style = stroke)
    }
}
