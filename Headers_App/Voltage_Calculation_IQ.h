/*
 * Voltage_Calculation_IQ.h
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

#ifndef HEADERS_VOLTAGE_CALCULATION_IQ_H_
#define HEADERS_VOLTAGE_CALCULATION_IQ_H_

//      -> it can not be necessary
//      -> TI examples like this
#include "IQmathLib.h"

/*
 *      -> If we use voltage divider, ACPL, INA(it is not add reference, it is use for impedance)
 *      -> DC side voltage resistors: 99k and 1kohm
 *      -> (1k/100k) = 1/100; voltage coefficient on voltage divider
 *      -> ANLG x (0.01) x 4095 / 3 = ADC
 *      -> ANLG = ADC x 100 x 3 / 4095
 *      -> ANLG = ADC x 0.07326
 *      -> When we look at DC side for example 300VDC,
 *      -> 300 x (0.01) = 3VDC = 4095 Digital Value
 *      -> 4095 x(0.07326) = 300VDC
 *      -> In this configuration we can read 300VDC
 */
//#define VDC_Sense_Coeff_Q19_Vltg_Clc _IQ19(0.07326)
#define VDC_Sense_Coeff_Q19_Vltg_Clc _IQ19(0.1159)

/*
 *      -> DC side voltage sensing
 *      -> use for exact value of actual voltage
 *      -> monitoring voltage sensor
 *      -> Voltage Divider, ACPL and INA826
 */
#define VDC_Voltage_Calculation_MACRO_Vltg_Clc(Voltage)         \
    Normalize_Voltage_Q19 = _IQ19(0.0);          \
    Temp_Voltage_Q19 = Voltage << 19;            \
    Normalize_Voltage_Q19 = _IQ19mpy(Temp_Voltage_Q19, VDC_Sense_Coeff_Q19_Vltg_Clc);           \

#endif /* HEADERS_VOLTAGE_CALCULATION_IQ_H_ */
