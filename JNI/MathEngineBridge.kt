package com.mathengine

import java.io.Closeable

/**
 * Ready-to-Use Android Kotlin Idiomatic Wrapper for the C11 Math Engine (MEP).
 */
class MathEngineKt : Closeable {
    private var handle: Long = MathEngineBridge.nativeCreate()

    init {
        check(handle != 0L) { "Failed to initialize native MEP Engine" }
    }

    fun eval(expr: String): MathEngineBridge.EvalResult =
        MathEngineBridge.EvalResult.parseWire(MathEngineBridge.nativeEval(handle, expr))

    fun openSession(): Int = MathEngineBridge.nativeSessionOpen(handle)
    fun closeSession(sessionId: Int): Boolean = MathEngineBridge.nativeSessionClose(handle, sessionId)
    fun resetSession(sessionId: Int): Boolean = MathEngineBridge.nativeSessionReset(handle, sessionId)

    fun evalSession(sessionId: Int, expr: String): MathEngineBridge.EvalResult =
        MathEngineBridge.EvalResult.parseWire(MathEngineBridge.nativeSessionEval(handle, sessionId, expr))

    fun exportSnapshots(sessionId: Int): String? =
        MathEngineBridge.nativeSessionExportSnapshots(handle, sessionId)

    fun restoreSession(snapshots: String): Int =
        MathEngineBridge.nativeSessionRestore(handle, snapshots)

    fun compileExt(mathSource: String): ByteArray? =
        MathEngineBridge.nativeCompileExt(handle, mathSource)

    fun loadMathc(bytes: ByteArray): Boolean =
        MathEngineBridge.nativeLoadMathc(handle, bytes)

    fun setModule(name: String, enabled: Boolean): Boolean =
        MathEngineBridge.nativeSetModule(handle, name, enabled)

    fun simplify(expr: String): String? =
        MathEngineBridge.nativeSymSimplify(handle, expr)

    fun diff(varName: String, expr: String): String? =
        MathEngineBridge.nativeSymDiff(handle, varName, expr)

    fun compactScratch(): Int =
        MathEngineBridge.nativeCompactScratch(handle)

    override fun close() {
        if (handle != 0L) {
            MathEngineBridge.nativeDestroy(handle)
            handle = 0L
        }
    }
}
