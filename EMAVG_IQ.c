/*
 * EMAVG_IQ.c
 *
 * Exponential Moving Average initialization function
 *
 *  Created on: Apr 29, 2019
 *      Author: furkan.dursun
 *
 *                            *****Furkan Dursun*****
 *
 *                  ******************** ********************
 *
 **************************Anova Project and Consulting*************************
 *
 */

#include "EMAVG_IQ.h"

/*
 *      -> initialize EMAVG filter struct in Q21 format
 *      -> use this before sampling starts
 *      -> the gain must adjust after execution of initialization function
 */

void Filter_Init_EMAVG(Variable_Q21_EMAVG *v) {
    v->In = _IQ21(0.0);
    v->Out = _IQ21(0.0);
    v->Gain_a = _IQ21(0.0);
}

