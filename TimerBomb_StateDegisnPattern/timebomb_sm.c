#include "uc_ao.h" /* uC/AO API */
#include "bsp.h"
#include "timebomb.h"

void TimeBomb_ctor(TimeBomb *const me) {
    Active_ctor(&me->super, (DispatchHandler)&TimeBomb_dispatch);
    timebombContext_Init(&me->_fsm, me);
    TimeEvent_ctor(&me->te, TIMEOUT_SIG, &me->super);
}

void TimeBomb_dispatch(TimeBomb *const me, Event const *const e) {
    /* temporary 'fsm' needed to fix the bug in SMC macro ENTRY_STATE() */
    struct timebombContext *fsm = &me->_fsm;
    switch (e->sig)
    {
    case INIT_SIG:
        timebombContext_EnterStartState(fsm);
        break;
    case BUTTON_PRESSED_SIG:
        timebombContext_BUTTON(fsm, e);
        break;
    case TIMEOUT_SIG:
        timebombContext_TIMEOUT(fsm, e);
        break;
    }
}

/* action functions... */
static inline void TimeBomb_ledRedOn(TimeBomb *const me) {
    BSP_ledRedOn();
}
static inline void TimeBomb_ledRedOff(TimeBomb *const me) {
    BSP_ledRedOff();
}
static inline void TimeBomb_ledBlueOn(TimeBomb *const me) {
    BSP_ledBlueOn();
}
static inline void TimeBomb_ledBlueOff(TimeBomb *const me) {
    BSP_ledBlueOff();
}
static inline void TimeBomb_ledGreenOn(TimeBomb *const me) {
    BSP_ledGreenOn();
}
static inline void TimeBomb_ledGreenOff(TimeBomb *const me) {
    BSP_ledGreenOff();
}
static inline void TimeBomb_setBlinkCtr(TimeBomb *const me, uint32_t blink_ctr) {
    me->blink_ctr = blink_ctr;
}
static inline void TimeBomb_decBlinkCtr(TimeBomb *const me) {
    --me->blink_ctr;
}
static inline void TimeBomb_armTE(TimeBomb *const me, uint32_t timeout, uint32_t interval) {
    TimeEvent_arm(&me->te, timeout, interval);
}

#include "timebomb_sm.h"

#define getOwner(fsm) (fsm)->_owner

static void TimeBombState_BUTTON(struct timebombContext *const fsm, Event const *const e) {
    getState(fsm)->Default(fsm);
}

static void TimeBombState_TIMEOUT(struct timebombContext *const fsm, Event const *const e) {
    getState(fsm)->Default(fsm);
}

static void TimeBombState_Default(struct timebombContext *const fsm) {
    State_Default(fsm);
}

#define TimeBombMap_wait4button_BUTTON TimeBombState_BUTTON
#define TimeBombMap_wait4button_TIMEOUT TimeBombState_TIMEOUT
#define TimeBombMap_wait4button_Default TimeBombState_Default
#define TimeBombMap_blink_BUTTON TimeBombState_BUTTON
#define TimeBombMap_blink_TIMEOUT TimeBombState_TIMEOUT
#define TimeBombMap_blink_Default TimeBombState_Default
#define TimeBombMap_pause_BUTTON TimeBombState_BUTTON
#define TimeBombMap_pause_TIMEOUT TimeBombState_TIMEOUT
#define TimeBombMap_pause_Default TimeBombState_Default
#define TimeBombMap_boom_BUTTON TimeBombState_BUTTON
#define TimeBombMap_boom_TIMEOUT TimeBombState_TIMEOUT
#define TimeBombMap_boom_Default TimeBombState_Default
#define TimeBombMap_DefaultState_BUTTON TimeBombState_BUTTON
#define TimeBombMap_DefaultState_TIMEOUT TimeBombState_TIMEOUT

void TimeBombMap_wait4button_Entry(struct timebombContext *const fsm) {
    struct TimeBomb *ctxt = getOwner(fsm);

    TimeBomb_ledGreenOn(ctxt);
}

void TimeBombMap_wait4button_Exit(struct timebombContext *const fsm) {
    struct TimeBomb *ctxt = getOwner(fsm);

    TimeBomb_ledGreenOff(ctxt);
}

#undef TimeBombMap_wait4button_BUTTON
static void TimeBombMap_wait4button_BUTTON(struct timebombContext *const fsm, Event const *const e) {
    struct TimeBomb *ctxt = getOwner(fsm);

    EXIT_STATE(getState(fsm));
    clearState(fsm);
    TimeBomb_setBlinkCtr(ctxt, 5U);
    setState(fsm, &TimeBombMap_blink);
    ENTRY_STATE(getState(fsm));
}

const struct TimeBombState TimeBombMap_wait4button = {
    TimeBombMap_wait4button_Entry,
    TimeBombMap_wait4button_Exit,
    TimeBombMap_wait4button_BUTTON,
    TimeBombMap_wait4button_TIMEOUT,
    TimeBombMap_wait4button_Default,
    0};

void TimeBombMap_blink_Entry(struct timebombContext *const fsm) {
    struct TimeBomb *ctxt = getOwner(fsm);

    TimeBomb_ledRedOn(ctxt);
    TimeBomb_armTE(ctxt, OS_TICKS_PER_SEC / 2, 0U);
}

void TimeBombMap_blink_Exit(struct timebombContext *const fsm) {
    struct TimeBomb *ctxt = getOwner(fsm);

    TimeBomb_ledRedOff(ctxt);
}

#undef TimeBombMap_blink_TIMEOUT
static void TimeBombMap_blink_TIMEOUT(struct timebombContext *const fsm, Event const *const e) {
    struct TimeBomb *ctxt = getOwner(fsm);

    EXIT_STATE(getState(fsm));
    clearState(fsm);
    TimeBomb_decBlinkCtr(ctxt);
    setState(fsm, &TimeBombMap_pause);
    ENTRY_STATE(getState(fsm));
}

const struct TimeBombState TimeBombMap_blink = {
    TimeBombMap_blink_Entry,
    TimeBombMap_blink_Exit,
    TimeBombMap_blink_BUTTON,
    TimeBombMap_blink_TIMEOUT,
    TimeBombMap_blink_Default,
    1};

void TimeBombMap_pause_Entry(struct timebombContext *const fsm)
{
    struct TimeBomb *ctxt = getOwner(fsm);

    TimeBomb_armTE(ctxt, OS_TICKS_PER_SEC / 2, 0U);
}

#undef TimeBombMap_pause_TIMEOUT
static void TimeBombMap_pause_TIMEOUT(struct timebombContext *const fsm, Event const *const e)
{
    struct TimeBomb *ctxt = getOwner(fsm);

    if (ctxt->blink_ctr > 0U)
    {
        EXIT_STATE(getState(fsm));
        /* No actions. */
        setState(fsm, &TimeBombMap_blink);
        ENTRY_STATE(getState(fsm));
    }
    else
    {
        EXIT_STATE(getState(fsm));
        setState(fsm, &TimeBombMap_boom);
        ENTRY_STATE(getState(fsm));
    }
}

const struct TimeBombState TimeBombMap_pause = {
    TimeBombMap_pause_Entry,
    NULL, /* Exit */
    TimeBombMap_pause_BUTTON,
    TimeBombMap_pause_TIMEOUT,
    TimeBombMap_pause_Default,
    2};

void TimeBombMap_boom_Entry(struct timebombContext *const fsm) {
    struct TimeBomb *ctxt = getOwner(fsm);

    TimeBomb_ledRedOn(ctxt);
    TimeBomb_ledGreenOn(ctxt);
    TimeBomb_ledBlueOn(ctxt);
}

const struct TimeBombState TimeBombMap_boom = {
    TimeBombMap_boom_Entry,
    NULL, /* Exit */
    TimeBombMap_boom_BUTTON,
    TimeBombMap_boom_TIMEOUT,
    TimeBombMap_boom_Default,
    3};

#ifdef NO_TIMEBOMB_SM_MACRO
void timebombContext_Init(struct timebombContext *const fsm, struct TimeBomb *const owner) {
    FSM_INIT(fsm, &TimeBombMap_wait4button);
    fsm->_owner = owner;
}

void timebombContext_EnterStartState(struct timebombContext *const fsm) {
    ENTRY_STATE(getState(fsm));
}

void timebombContext_BUTTON(struct timebombContext *const fsm, Event const *const e) {
    const struct TimeBombState *state = getState(fsm);

    assert(state != NULL);
    setTransition(fsm, "BUTTON");
    state->BUTTON(fsm, e);
    setTransition(fsm, NULL);
}

void timebombContext_TIMEOUT(struct timebombContext *const fsm, Event const *const e) {
    const struct TimeBombState *state = getState(fsm);

    assert(state != NULL);
    setTransition(fsm, "TIMEOUT");
    state->TIMEOUT(fsm, e);
    setTransition(fsm, NULL);
}
#endif