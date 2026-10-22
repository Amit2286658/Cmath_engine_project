package com.mathengine;

/**
 * Ready-to-Use Android Java Wrapper for the C11 Math Engine (MEP).
 *
 * Wraps the native JNI lifecycle, stateless evaluation, parallel stateful
 * sessions, order-tagged FNV-1a snapshot export/restore, .math extension
 * compilation, .mathc binary cache loading, O(1) module toggling, symbolic
 * differentiation/simplification, and scratch pool Mark-Sweep compaction.
 */
public final class MathEngineBridge implements AutoCloseable {
    static {
        System.loadLibrary("mathengine");
    }

    public static final class EvalResult {
        public final boolean ok;
        public final String type;
        public final String unit;
        public final String value;
        public final String error;

        private EvalResult(boolean ok, String type, String unit, String value, String error) {
            this.ok    = ok;
            this.type  = type;
            this.unit  = unit;
            this.value = value;
            this.error = error;
        }

        public static EvalResult parseWire(String wire) {
            if (wire == null) {
                return new EvalResult(false, "void", "none", "", "Null JNI response");
            }
            if (wire.startsWith("OK|")) {
                int p1 = wire.indexOf('|', 3);
                int p2 = (p1 > 0) ? wire.indexOf('|', p1 + 1) : -1;
                if (p1 > 0 && p2 > p1) {
                    return new EvalResult(
                        true,
                        wire.substring(3, p1),
                        wire.substring(p1 + 1, p2),
                        wire.substring(p2 + 1),
                        null
                    );
                }
            }
            String err = wire.startsWith("ERR|") ? wire.substring(4) : wire;
            return new EvalResult(false, "void", "none", "", err);
        }
    }

    private long handle;

    public MathEngineBridge() {
        this.handle = nativeCreate();
        if (this.handle == 0L) {
            throw new IllegalStateException("Failed to initialize native MEP Engine");
        }
    }

    public EvalResult eval(String expr) {
        return EvalResult.parseWire(nativeEval(handle, expr));
    }

    public int openSession() {
        return nativeSessionOpen(handle);
    }

    public boolean closeSession(int sessionId) {
        return nativeSessionClose(handle, sessionId);
    }

    public boolean resetSession(int sessionId) {
        return nativeSessionReset(handle, sessionId);
    }

    public EvalResult evalSession(int sessionId, String expr) {
        return EvalResult.parseWire(nativeSessionEval(handle, sessionId, expr));
    }

    public String exportSessionSnapshots(int sessionId) {
        return nativeSessionExportSnapshots(handle, sessionId);
    }

    public int restoreSession(String newlineJoinedSnapshots) {
        return nativeSessionRestore(handle, newlineJoinedSnapshots);
    }

    public byte[] compileExtension(String mathSource) {
        return nativeCompileExt(handle, mathSource);
    }

    public boolean loadMathc(byte[] mathcBytes) {
        return nativeLoadMathc(handle, mathcBytes);
    }

    public boolean setModule(String moduleName, boolean enabled) {
        return nativeSetModule(handle, moduleName, enabled);
    }

    public boolean isModuleActive(String moduleName) {
        return nativeIsModuleActive(handle, moduleName);
    }

    public String simplify(String expr) {
        return nativeSymSimplify(handle, expr);
    }

    public String diff(String varName, String expr) {
        return nativeSymDiff(handle, varName, expr);
    }

    public int compactScratch() {
        return nativeCompactScratch(handle);
    }

    public int scratchPoolUsed() {
        return nativeScratchPoolUsed(handle);
    }

    public String lastError() {
        return nativeLastError(handle);
    }

    @Override
    public void close() {
        if (handle != 0L) {
            nativeDestroy(handle);
            handle = 0L;
        }
    }

    public static native long    nativeCreate();
    public static native long    nativeCreateBlank();
    public static native void    nativeDestroy(long handle);
    public static native String  nativeEval(long handle, String expr);
    public static native String  nativeLastError(long handle);
    public static native int     nativeSessionOpen(long handle);
    public static native boolean nativeSessionClose(long handle, int sessionId);
    public static native boolean nativeSessionReset(long handle, int sessionId);
    public static native String  nativeSessionEval(long handle, int sessionId, String expr);
    public static native int     nativeSessionSnapshotCount(long handle, int sessionId);
    public static native String  nativeSessionSnapshotAt(long handle, int sessionId, int index);
    public static native String  nativeSessionExportSnapshots(long handle, int sessionId);
    public static native int     nativeSessionRestore(long handle, String newlineJoinedSnapshots);
    public static native byte[]  nativeCompileExt(long handle, String mathSource);
    public static native boolean nativeLoadMathc(long handle, byte[] mathcBytes);
    public static native boolean nativeSetModule(long handle, String modName, boolean enabled);
    public static native boolean nativeIsModuleActive(long handle, String modName);
    public static native String  nativeSymSimplify(long handle, String expr);
    public static native String  nativeSymDiff(long handle, String varName, String expr);
    public static native int     nativeCompactScratch(long handle);
    public static native int     nativeScratchPoolUsed(long handle);
}
