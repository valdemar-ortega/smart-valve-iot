package com.smartvalve.app

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider

/** Minimal ViewModel factory, so the app needs no dependency-injection framework. */
class SimpleFactory(private val creator: () -> ViewModel) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T = creator() as T
}
