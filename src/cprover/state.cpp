/*******************************************************************\

Module: State Encoding

Author: Daniel Kroening, dkr@amazon.com

\*******************************************************************/

#include "state.h"

#include <util/pointer_expr.h>

const irep_idt ID_state_is_cstring = "state_is_cstring";
const irep_idt ID_state_is_dynamic_object = "state_is_dynamic_object";
const irep_idt ID_state_live_object = "state_live_object";
const irep_idt ID_state_object_size = "state_object_size";
const irep_idt ID_state_r_ok = "state_r_ok";
const irep_idt ID_state_w_ok = "state_w_ok";
const irep_idt ID_state_rw_ok = "state_rw_ok";
