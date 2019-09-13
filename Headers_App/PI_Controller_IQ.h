/*
 * PI_Controller_Trial_IQ.h
 *
 *  Created on: Apr 29, 2018
 *      Author: furkan.dursun
 *
 *                            *****Furkan Dursun*****
 *
 *                  ******************** ********************
 *
 **************************Anova Project and Consulting*************************
 *
 */

#ifndef HEADERS_PI_CONTROLLER_IQ_H_
#define HEADERS_PI_CONTROLLER_IQ_H_

#include "IQmathLib.h"

#define Input_Saturation _IQ22(250.0)

/*
 *      -> Coefficient for PI_Controller
 *      -> Q22 calculation
 */
typedef struct {
    long Input;
    long Ki;
    long Ki_Side_Constant;
    long Ki_Side[2];
    long Kp;
    long Kp_Side;
    long Output_Max;
    long Output_Min;
    long Output;

} Variable_Q22_PI_Controller;

//      -> Definition for Init function
void Control_Init_PI_Controller(Variable_Q22_PI_Controller *v);

/*
 *      -> While calculation of reference dq and reference abc component,
 *      -> this control block used several times
 *      -> Voltage Controller, Current Controller
 *      -> Q22 operation
 */
#define Control_MACRO_PI_Controller(v)          \
    v.Ki_Side[1] = _IQsat(_IQ22mpy(v.Input, v.Ki_Side_Constant) + v.Ki_Side[0], Input_Saturation, -Input_Saturation);           \
    v.Kp_Side = _IQ22mpy(v.Input ,v.Kp );            \
    v.Output = _IQsat(v.Ki_Side[1] + v.Kp_Side, v.Output_Max, v.Output_Min);            \
    v.Ki_Side[0] = v.Ki_Side[1];            \

#endif /* HEADERS_PI_CONTROLLER_IQ_H_ */
