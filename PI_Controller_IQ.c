/*
 * PI_Controller_Trial_IQ.c
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


#include "PI_Controller_IQ.h"

/*
 *      -> Initialize PI_Controller struct
 *      -> Use this before sampling starts
 */
void Control_Init_PI_Controller(Variable_Q22_PI_Controller *v) {
    v->Input = _IQ22(0.0);
    v->Ki = _IQ22(0.0);
    v->Ki_Side_Constant = _IQ22(0.0);
    v->Ki_Side[0] = _IQ22(0.0);
    v->Ki_Side[1] = _IQ22(0.0);
    v->Kp = _IQ22(0.0);
    v->Kp_Side = _IQ22(0.0);
    v->Output_Max = _IQ22(0.0);
    v->Output_Min = _IQ22(0.0);
    v->Output = _IQ22(0.0);
}

