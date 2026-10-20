/*
 * session.h — Independent Parallel In-Memory Sessions (`Core/session/`)
 *
 * Purpose:
 *   Defines the data structures and lifecycle/evaluation API for isolated,
 *   unnamed parallel in-memory evaluation sessions (`eng_session_open`,
 *   `eng_session_close`, `eng_session_reset`, `eng_session_eval`).
 */

#ifndef SESSION_H
#define SESSION_H

#include "mem_arena.h"
#include "sym_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine Engine;

/* ============================================================================
 * 1. PARALLEL IN-MEMORY SESSION CONSTANTS & DATA STRUCTURES
 * ============================================================================ */

#define ENG_MAX_SESSIONS          8
#define ENG_SESSION_MAX_EVALS    32
#define ENG_SESSION_MAX_VARS     16
#define ENG_SNAPSHOT_MAX_LEN    512

typedef struct {
    char name[32];            /* Variable identifier                           */
    char type_name[32];       /* Current runtime entity type name              */
    char unit_name[32];       /* Current runtime unit group (or vec elem unit) */
    char lock_type_name[32];  /* Concrete type lock ("*" if dynamic)           */
    char lock_unit_name[32];  /* Concrete unit lock ("*" if dynamic)           */
    char val_text[256];       /* Canonical MEP expression for current value    */
} EngSessionVar;

typedef struct {
    bool          in_use;
    uint16_t      eval_count;
    uint16_t      snap_iter_cursor;
    uint32_t      chain_hash;
    uint8_t       var_count;
    EngSessionVar vars[ENG_SESSION_MAX_VARS];
    char          snapshots[ENG_SESSION_MAX_EVALS][ENG_SNAPSHOT_MAX_LEN];
} EngSession;

/* ============================================================================
 * 2. PARALLEL SESSION LIFECYCLE & STATEFUL EVALUATION API
 * ============================================================================ */

/*
 * Allocate a new unnamed, independent in-memory session.
 * Returns a session handle (`0 .. ENG_MAX_SESSIONS - 1`), or `-1` if all slots are full.
 */
int  eng_session_open(Engine *eng);

/* Close an active in-memory session and release its slot */
bool eng_session_close(Engine *eng, int session_id);

/* Reset an active session's variables and evaluation snapshots while keeping it open */
bool eng_session_reset(Engine *eng, int session_id);

/*
 * Evaluate `expr` within `session_id`:
 *   - Restores all variables, type/unit locks, containers, structs, and lambdas
 *     from prior evaluations in `session_id`.
 *   - Persists updated/newly declared variables back into `session_id`.
 *   - Records a new order-tagged raw-text snapshot for this evaluation.
 */
bool eng_session_eval(Engine *eng, int session_id, const char *expr, Value *out_val);

#ifdef __cplusplus
}
#endif

#endif /* SESSION_H */
