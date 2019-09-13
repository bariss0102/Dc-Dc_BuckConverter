/*
 * EMAVG_IQ.h
 *
 * Exponential Moving Average definitions and macro
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

#ifndef HEADERS_EMAVG_IQ_H_
#define HEADERS_EMAVG_IQ_H_

#include "IQmathLib.h"

//      -> coefficient for Exponential Moving Average
//		-> Q21 operation
typedef struct {
    long In;
    long Out;
    long Gain_a;
} Variable_Q21_EMAVG;

//      -> Definition for init function
void Filter_Init_EMAVG(Variable_Q21_EMAVG *v);

/*
 *      -> Q21 operation
 *      -> this block calculate the filtered version of signal
 *      -> In AC DC Project, it is used for exponential moving average of DC voltage
 */
#define Filter_MACRO_EMAVG(v)           \
    v.Out = _IQ21mpy(v.In - v.Out, v.Gain_a) + v.Out;            \

#endif /* HEADERS_EMAVG_IQ_H_ */
