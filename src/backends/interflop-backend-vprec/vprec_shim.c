#include "interflop_vprec.h"

void interflop_vprec_enter_function(interflop_function_stack_t *stack,
                                     void *context, int nb_args, ...) {
    (void)stack; (void)context; (void)nb_args;
}

void interflop_vprec_exit_function(interflop_function_stack_t *stack,
                                    void *context, int nb_args, ...) {
    (void)stack; (void)context; (void)nb_args;
}

void interflop_vprec_user_call(void *context, interflop_call_id id, ...) {
    (void)context; (void)id;
}
