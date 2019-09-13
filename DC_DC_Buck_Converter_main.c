/*
 * DC_DC_Buck_Converter_main.c
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

/*
 ******      DC_DC_Buck_Converter    *******
 ******                                              *******
     -> In DC_DC_Buck_Converter we will perform to control buck_boost converter

     -> This project prepared for matlab and DSP F28335 co-simulation

     -> The control algorithm run on DSP with manually coded algorithm and the power electronic circuit simulated
        on matlab/simulink

     -> The algorithm is untested and not optimized. It is coded for only to learn whether or not simulink and
        DSP work together in the scenario what we want

     -> Probably system will perform slow response
*/



/*
 *      -> Adding library
 *      -> "DSP28x_Project.h" and "IQmathLib.h" are prepared by Texas Insturements
 *      -> Others are written for specific Q value operation, must be careful any change
 */
#include "DSP28x_Project.h"
#include "IQmathLib.h"
#include "Voltage_Calculation_IQ.h"
#include "PI_Controller_IQ.h"
#include "EMAVG_IQ.h"

/*
 *      -> We compare this with actual dc value, dc value is Q19 format
 *      -> Used in voltage controller side
 */
#define DC_ref_Q19_Vltg_Contrl _IQ19(50.0)

//      -> This must be max. output value of PI controller
#define Vp_Ref_Q21 _IQ21(200.0)

//      -> 1/Vpp, 1/400
//      -> We will use this for DutyA
#define One_Divided_Vpp_Ref_Q21 _IQ21(0.0025)

/*
 *      -> Init system,adc, interrupt services etc.
 *      -> Texas Insturements ready function
 */
extern void InitAdc(void);
extern void InitSysCtrl(void);
extern void InitPieCtrl(void);
extern void InitPieVectTable(void);

/*
 *       -> Function which are exist in Three_Phase_Trial_0.1_main.c
 */
void Gpio_select(void);     //  -> Gpio settings for input and output, pins mode selection
void Adc_Setup(void);       //  -> Adc block configuration
void ADC_and_System_PWM_Setup(void);    //  -> Setup for triggering adc and base system interrupt
void Switch_PMW_Setup(void);        //  -> Switching pwm channels setup, which are ePWM1, ePWM2, ePWM3
void Init_Zero_Condition(void);     //  -> zero condition assignment
                                    //  -> Local variable initial value and
                                    //  -> variable initial value of struct with init func.

interrupt void epwm4_base_isr(void);    //  -> Base interrupt trigger from PWM4
                                        //  -> All closed loop calculation realize in here

interrupt void cpu_timer0_isr(void);  //Boot Sequence
/*****
 *      -> Global Variable Declaration
 *****/


/*
 *      -> These are used for assign calculated actual value
 *      -> These consist actual phase value
 *      -> Q19 format, result of Voltage and Current Calculation
 */
volatile long ANLG_PH_DC_Q19;     //  -> Normalize and actual value of DC voltage and current respectively
volatile long ANLG_PH_DC_Avg_Q19;

//      -> used for voltage normalization macro operation
volatile long Temp_Voltage_Q19;
volatile long Normalize_Voltage_Q19;

volatile long Control_Ref_Q21;

/*
 *      -> Duty cycle for ePWM1(2)(3)A channels
 *      -> ePWM1(2)(3)B channels adjust with pwm deadband module
 *      -> Q21 format
 */
volatile long DutyA_Q21_Swtch_Contrl;
volatile long DutyB_Q21_Swtch_Contrl;
volatile long DutyC_Q21_Swtch_Contrl;

volatile long CMPA_Overflow_1_Swtch_Contrl;

Variable_Q21_EMAVG var_DC_Avg_Q21_EMAVG;

/*
 *      -> First PI_Controller of Voltage regulation
 *      -> Q22 format
 */
Variable_Q22_PI_Controller var1_Q22_PI_Controller_Vltg_Contrl;

//      -> Interrupt counter used for interrupt frequency control GPIO31
unsigned int interrupt_counter_test, TimerFlag=0;
const unsigned long int Delay = 10000000; //10 seconds

/*
****************                                       ****************
********************** Beginnig of Main Function **********************
****************                                       ****************
 */

void main(void) {

    InitSysCtrl();      //  -> Basic Core Init

    DINT;       //  -> Disable all interrupt

    InitPieCtrl();      //  -> Basic setup of PIE table

    IER = 0x0000;       //  -> Interrupt Enable Register,
                        //  -> contains enable bits for all the maskable CPU interrupt levels

    IFR = 0x0000;       //  -> Interrupt Flag Register,
                        //  -> used for identify and clear pending interrupt

    InitPieVectTable();     //  -> default ISR's int PIE

    //      -> for know we don't reset ADC Block, when system is reset ADC block reset automatically
    //    AdcRegs.ADCTRL1.bit.RESET = 1;      //      -> to reset whole adc block, use delay two adc clock
    //    asm("RPT #22 || NOP");      //      -> bad term, main.obj warning(not always)


    InitAdc();      //  -> Initialization of adc block

    Gpio_select();  //  -> Gpio settings for input and output, pins mode selection

    Adc_Setup();    //  -> Adc block configuration

    ADC_and_System_PWM_Setup();     //  -> Setup for triggering adc and base system interrupt
                                    //  -> closed loop interrupt and adc soc provide form ePWM4 channels

    Switch_PMW_Setup();     //  -> Switching pwm channels setup, which are ePWM1, ePWM2, ePWM3

    Init_Zero_Condition();      //      -> zero condition assignment
                                //      -> Local variable initial value and
                                //      -> variable initial value of struct with init func.

    EALLOW;
    PieVectTable.EPWM4_INT = &epwm4_base_isr;       //      -> Connection of ePWM4 interrupt
    PieVectTable.TINT0 = &cpu_timer0_isr;
    EDIS;

    InitCpuTimers();    //Start timers
    ConfigCpuTimer(&CpuTimer0, 150, Delay);
    CpuTimer0Regs.TCR.all = 0x4000; //write-only instruction to set TSS bit = 0


    IER |= M_INT3;      //  -> 0000 0000 0000 0100 -> 0x0004
                        //  -> Enable CPU INT3 which is connected to EPWM1-6 INT
                        //  -> M_INT3 = 4
    IER |= M_INT1;      //For timer0

    PieCtrlRegs.PIEIER3.bit.INTx4 = 1;  //Pwm
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;  //Timer

    EINT;       //  -> Enable maskable interrupt
    ERTM;       //  -> Enable debug event

    while(1)        //  -> Infinite loop for device working
    {
    }

}

//  -> Gpio settings for input and output, pins mode selection
void Gpio_select(void) {
    EALLOW;
    GpioCtrlRegs.GPAMUX1.all = 0;           //  -> GPIO15 ... GPIO0 = General Puropse I/O
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;     //  -> ePWM1A active
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 1;     //  -> ePWM1B active
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;     //  -> ePWM2A active
    GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 1;     //  -> ePWM2B active
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;     //  -> ePWM3A active
    GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 1;     //  -> ePWM3B active
    GpioCtrlRegs.GPAMUX2.all = 0;           //  -> GPIO31 ... GPIO16 = General Purpose I/O

    GpioCtrlRegs.GPBMUX1.all = 0;           //  -> GPIO47 ... GPIO32 = General Purpose I/O
    GpioCtrlRegs.GPBMUX2.all = 0;           //  -> GPIO63 ... GPIO48 = General Purpose I/O

    GpioCtrlRegs.GPCMUX1.all = 0;           //  -> GPIO79 ... GPIO64 = General Purpose I/O
    GpioCtrlRegs.GPCMUX2.all = 0;           //  -> GPIO87 ... GPIO80 = General Purpose I/O

    GpioCtrlRegs.GPADIR.all = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;     //  1 -> GPIO31 as output, LED

    GpioCtrlRegs.GPBDIR.all = 0;            //  -> GPIO63-33 as inputs
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;     //  1 -> GPIO34 as output, LED
    GpioCtrlRegs.GPBDIR.bit.GPIO32 = 1;     //  1 -> GPIO32 as output, pwm running flag.

    GpioCtrlRegs.GPCDIR.all = 0;            //  -> GPIO87-64 as inputs
    EDIS;
}

//  -> Adc block configuration
void Adc_Setup(void) {
    AdcRegs.ADCTRL1.all = 0;
    AdcRegs.ADCTRL1.bit.ACQ_PS = 0;         //  0 -> 1 x ADCCLK, sampling window
    AdcRegs.ADCTRL1.bit.SEQ_CASC = 1;       //  1 -> cascaded sequencer
    AdcRegs.ADCTRL1.bit.CPS = 0;            //  0 -> divide by 1 for clk, last element of division adc clock
    AdcRegs.ADCTRL1.bit.CONT_RUN = 1;       //  1  ->continuous conversion mode
    AdcRegs.ADCTRL1.bit.SEQ_OVRD = 0;       //  0 ->sequencer to wrap around at the end of conversions

    AdcRegs.ADCTRL2.all = 0;
    AdcRegs.ADCTRL2.bit.INT_ENA_SEQ1 = 1;       // 1 -> enable SEQ1 interrupt
    AdcRegs.ADCTRL2.bit.EPWM_SOCA_SEQ1 = 1;     //  1 -> SEQ1 start from ePWM_SOC trigger
    AdcRegs.ADCTRL2.bit.INT_MOD_SEQ1 = 0;       //  0 -> interrupt after ever end of sequence

    AdcRegs.ADCTRL3.bit.ADCCLKPS = 3;       //  -> ADC clock: FCLK = HSPCLK / 2 * ADCCLKPS
                                            //  ->  HSPCLK = 75MHz (see DSP2833x_SysCtrl.c)
                                            //  ->FCLK = 12.5 MHz
    AdcRegs.ADCTRL3.bit.SMODE_SEL = 1;      //  1-> Simultaneous Sampling, two sampling at same time

//      -> first quaternary conversion
    AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0;        //  -> ADCIN0, ADCINB0 ->
    AdcRegs.ADCCHSELSEQ1.bit.CONV01 = 1;        //  -> ADCIN1, ADCINB1 ->
    AdcRegs.ADCCHSELSEQ1.bit.CONV02 = 2;        //  -> ADCIN2, ADCINB2 ->
    AdcRegs.ADCCHSELSEQ1.bit.CONV03 = 3;        //  -> ADCIN3, ADCINB3 -> ANLG_PH_DC, DC_VIOUT (Result6, Result7)

    //      -> second quaternary conversion
    AdcRegs.ADCCHSELSEQ2.bit.CONV04 = 0;        //  -> ADCIN0, ADCINB0 ->
    AdcRegs.ADCCHSELSEQ2.bit.CONV05 = 1;        //  -> ADCIN1, ADCINB1 ->
    AdcRegs.ADCCHSELSEQ2.bit.CONV06 = 2;        //  -> ADCIN2, ADCINB2 ->
    AdcRegs.ADCCHSELSEQ2.bit.CONV07 = 3;        //  -> ADCIN3, ADCINB3 -> ANLG_PH_DC, DC_VIOUT (Result14, Result15)

    AdcRegs.ADCMAXCONV.bit.MAX_CONV1 = 0x0007;      //      -> 8 pair of conversion
}

//  -> Setup for triggering adc and base system interrupt
void ADC_and_System_PWM_Setup(void) {
    //      -> maybe shadow register can be use
    //      -> Setup EPwm4 for adc and system base interrupt
    EPwm4Regs.TBCTL.bit.FREE_SOFT = 11;     //      11 -> ignore emulation suspend
    EPwm4Regs.TBCTL.bit.PHSDIR = 0;     //  0 -> count down after sync event, we don't use for now
    EPwm4Regs.TBCTL.bit.CLKDIV = 0;     //  000 -> TBCLK = HSPCLK/1
    EPwm4Regs.TBCTL.bit.HSPCLKDIV = 0;  //  -> HSPCLK = SYSCLKOUT/1
    EPwm4Regs.TBCTL.bit.SWFSYNC = 0;    //  0 -> no software sync produced
    EPwm4Regs.TBCTL.bit.SYNCOSEL = 3;   //  3 -> disable sync
    EPwm4Regs.TBCTL.bit.PRDLD = 0;      //  0 -> reload PRD on counter  when tbctr is equal to zero
                                        //  -> shadow register use for now
    EPwm4Regs.TBCTL.bit.PHSEN = 0;      //  0 -> phase control disabled
    EPwm4Regs.TBCTL.bit.CTRMODE = 2;    //  2 -> up-down-count mode

    /*
    EPwm4Regs.TBPRD = 375;      //      -> 375 = 1/2 x 150MHz / 200kHz
                                //      375 -> 200kHz sampling
    */

    /*
    EPwm4Regs.TBPRD = 1500;     //      -> 1500 = 1/2 x 150Mz / 50kHz
                                //      1500 -> 50kHz sampling
    */

    EPwm4Regs.TBPRD = 750;      //      -> 750 = 1/2 x 150Mhz / 100kHz
                                //      -> 750 -> 100 kHz sampling

    EPwm4Regs.ETPS.bit.SOCBCNT = 0;     //  0 -> no event have occured
    EPwm4Regs.ETPS.bit.SOCBPRD = 0;     //  0 -> disable the SOCB event counter
    EPwm4Regs.ETPS.bit.SOCACNT = 0;     //  0 -> no event have occured
    EPwm4Regs.ETPS.bit.SOCAPRD = 1;     //  1 -> generate SOCA on first event
    EPwm4Regs.ETPS.bit.INTCNT = 0;      //  0 -> no event have occured
    EPwm4Regs.ETPS.bit.INTPRD = 1;      //  1 -> generate an interrupt on the first event

    EPwm4Regs.ETSEL.bit.SOCBEN = 0;     //  0 -> disable SOCB
    EPwm4Regs.ETSEL.bit.SOCBSEL = 0;    //  0 -> don't care
    EPwm4Regs.ETSEL.bit.SOCAEN = 1;     //  1 -> enable SOCA
    EPwm4Regs.ETSEL.bit.SOCASEL = 1;    //  1 -> generate SOCA on time-base counter equal to zero
    EPwm4Regs.ETSEL.bit.INTEN = 1;      //  1 -> Enable EPWMx_INT generation
    EPwm4Regs.ETSEL.bit.INTSEL = 2;     //  2 -> generate interrupt on time-base counter equal to period
}

//  -> Switching pwm channels setup, which are ePWM1, ePWM2, ePWM3
void Switch_PMW_Setup(void) {
    EPwm1Regs.TBCTL.bit.FREE_SOFT = 11; //  11 -> ignore emulation suspend
    EPwm1Regs.TBCTL.bit.PHSDIR = 0;     //  0 -> count down after sync event, we don't use for now
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;     //  000 -> TBCLK = HSPCLK/1
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;  //  -> HSPCLK = SYSCLKOUT/1
    EPwm1Regs.TBCTL.bit.SWFSYNC = 0;    //  0 -> no software sync produced
    EPwm1Regs.TBCTL.bit.SYNCOSEL = 1;   //  1 -> when CTR = zero synchronization output
    EPwm1Regs.TBCTL.bit.PRDLD = 0;      //  0 -> reload PRD on counter  when tbctr is equal to zero
    EPwm1Regs.TBCTL.bit.PHSEN = 0;      //  0 -> phase control disabled
    EPwm1Regs.TBCTL.bit.CTRMODE = 2;    //  2 -> up-down-count mode

    EPwm1Regs.TBPRD = 750;      //  -> 750 = 1/2 x 150Mhz / 100kHz
                                //  -> 750 -> 100 kHz pwm signal
                                //  -> 3750 = 1/2 x 150MHz / 20kHz
                                //  -> 3750 -> 20 kHz pwm signal

                                //  -> !!! Attention !!! <-
                                //  -> If you change this you must also change DBRED, DBFED register

    EPwm1Regs.ETPS.bit.SOCBCNT = 0;     //  0 -> no event have occured
    EPwm1Regs.ETPS.bit.SOCBPRD = 0;     //  0 -> disable the SOCB event counter
    EPwm1Regs.ETPS.bit.SOCACNT = 0;     //  0 -> no event have occured
    EPwm1Regs.ETPS.bit.SOCAPRD = 0;     //  0 -> disable the SOCA event counter
    EPwm1Regs.ETPS.bit.INTCNT = 0;      //  0 -> no event have occured
    EPwm1Regs.ETPS.bit.INTPRD = 0;      //  0 -> disable the interrupt event counter

    EPwm1Regs.ETSEL.bit.SOCBEN = 0;     //  0 -> disable SOCB
    EPwm1Regs.ETSEL.bit.SOCBSEL = 0;    //  0 -> don't care
    EPwm1Regs.ETSEL.bit.SOCAEN = 0;     //  0 -> disable EPWMxSOCA
    EPwm1Regs.ETSEL.bit.SOCASEL = 0;    //  0 -> reserved
    EPwm1Regs.ETSEL.bit.INTEN = 0;      //  0 -> disable EPWMx_INT generation
    EPwm1Regs.ETSEL.bit.INTSEL = 0;     //  0 -> Reserved

    EPwm1Regs.AQCTLA.bit.CAU = 2;       //  2-> Pwm output high when counter is incrementing and
                                        //  counter equals the active CMPA register
    EPwm1Regs.AQCTLA.bit.CAD = 1;       //  1-> Pwm output low when counter is decrementing and
                                        //  -> counter equals the active CMPA register

//    EPwm1Regs.DBRED = 50;      //  -> Dead band on rising edge -> DBRED x 1/(TBCLK) = 1/(150Mhz)x100
//                                //                           -> 133.33ns
//                                //  -> Dead band on rising edge -> DBRED x 1/(TBCLK) = 1/(150Mhz)x100
//                                //                           -> 666.66ns
//
//    EPwm1Regs.DBFED = 50;      //  Dead band on falling edge -> DBFED x 1/(TBCLK) = 1/(150Mhz)x100
//                                //                            -> 133.33ns
//                                //  -> Dead band on rising edge -> DBRED x 1/(TBCLK) = 1/(150Mhz)x100
//                                //                           -> 666.66ns

    EPwm1Regs.DBCTL.bit.IN_MODE = 0;    //  0-> EPWMxA is source for both falling and rising edge
    EPwm1Regs.DBCTL.bit.POLSEL = 1;     //  1-> Active low complementry, A channel is inverted
                                        //  -> Although we didn't adjust AQCTLB, EPWMxB channel generate signal,
                                        //  -> Because we use channel A as source for channel B
    EPwm1Regs.DBCTL.bit.OUT_MODE = 3;   //  3-> Dead band is fully enabled for both rising and falling edge

    EALLOW;     //      -> All of trip-zone register, eallow protected
    EPwm1Regs.TZCTL.bit.TZA = 1;    //  -> EPWMxA high state when trip-zone event occur
    EPwm1Regs.TZCTL.bit.TZB = 1;    //  -> EPWMxB high state when trip-zone event occur

    EPwm1Regs.TZEINT.bit.OST = 0;   //  -> Disable one-shot interrupt generation
                                    //  -> We use ost event but don't want interrupt services
    EPwm1Regs.TZEINT.bit.CBC = 0;   //  -> Disable cycle-by-cycle interrupt generation
    EDIS;

/*
 *      -> Very first initial condition is in this format:
 *      Phase A up switch open, dowm switch close
 *      Phase B up switch close, dowm switch open
 *      Phase C up switch close, down switch open
 */
    EPwm1Regs.CMPA.half.CMPA = 0;   //  -> We can not wait Init_Zero_Condition function for pwm output
                                    //  -> first duty cycle of EPWM1A is %0
                                    //  -> first duty cycle of EPWM1b is %100

    EPwm2Regs.TBCTL.bit.FREE_SOFT = 11; //      11 -> ignore emulation suspend
    EPwm2Regs.TBCTL.bit.PHSDIR = 0;     //  0 -> Count down after the synchronization event
    EPwm2Regs.TBCTL.bit.CLKDIV = 0;     //  000 -> TBCLK = HSPCLK/1
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = 0;  //  -> HSPCLK = SYSCLKOUT/1
    EPwm2Regs.TBCTL.bit.SWFSYNC = 0;    //  0 -> no software sync produced
    EPwm2Regs.TBCTL.bit.SYNCOSEL = 0;   //  0 -> EPWMxSYNC synchronization output
    EPwm2Regs.TBCTL.bit.PRDLD = 0;      //  0 -> reload PRD on counter  when tbctr is equal to zero
    EPwm2Regs.TBCTL.bit.PHSEN = 1;      //  1 -> load the time-base counter with the phase register
                                        //       when an EPWMxSYNCI input signal occurs
    EPwm2Regs.TBCTL.bit.CTRMODE = 2;    //  2 -> up-down-count mode

    EPwm2Regs.TBPRD = 750;      //  -> 750 = 1/2 x 150Mhz / 100kHz
                                //  -> 750 -> 100 kHz pwm signal
                                //  -> 3750 = 1/2 x 150MHz / 20kHz
                                //  -> 3750 -> 20 kHz pwm signal

                                //  -> !!! Attention !!! <-
                                //  -> If you change this you must also change DBRED, DBFED register

    EPwm2Regs.TBPHS.half.TBPHS =500;    //  500 -> TBPRD = 750 and 1/3 120 degree phase shift
                                        //         with PHSDIR = 0

    EPwm2Regs.ETPS.bit.SOCBCNT = 0;     //  0 -> no event have occured
    EPwm2Regs.ETPS.bit.SOCBPRD = 0;     //  0 -> disable the SOCB event counter
    EPwm2Regs.ETPS.bit.SOCACNT = 0;     //  0 -> no event have occured
    EPwm2Regs.ETPS.bit.SOCAPRD = 0;     //  0 -> disable the SOCA event counter
    EPwm2Regs.ETPS.bit.INTCNT = 0;      //  0 -> no event have occured
    EPwm2Regs.ETPS.bit.INTPRD = 0;      //  0 -> disable the interrupt event counter

    EPwm2Regs.ETSEL.bit.SOCBEN = 0;     //  0 -> disable SOCB
    EPwm2Regs.ETSEL.bit.SOCBSEL = 0;    //  0 -> don't care
    EPwm2Regs.ETSEL.bit.SOCAEN = 0;     //  0 -> disable EPWMxSOCA
    EPwm2Regs.ETSEL.bit.SOCASEL = 0;    //  0 -> reserved
    EPwm2Regs.ETSEL.bit.INTEN = 0;      //  0 -> disable EPWMx_INT generation
    EPwm2Regs.ETSEL.bit.INTSEL = 0;     //  0 -> Reserved

    EPwm2Regs.AQCTLA.bit.CAU = 2;       //  2-> Pwm output high when counter is incrementing and
                                        //  counter equals the active CMPA register
    EPwm2Regs.AQCTLA.bit.CAD = 1;       //  1-> Pwm output low when counter is decrementing and
                                        //  -> counter equals the active CMPA register

    EPwm2Regs.DBRED = 50;      //  Dead band on rising edge -> DBRED x 1/(TBCLK) = 1/(150Mhz)x100
                                //                           -> 133.33ns
                                //  -> Dead band on rising edge -> DBRED x 1/(TBCLK) = 1/(150Mhz)x100
                                //                           -> 666.66ns
    EPwm2Regs.DBFED = 50;      //  Dead band on falling edge -> DBFED x 1/(TBCLK) = 1/(150Mhz)x100
                                //                            -> 133.33ns
                                //  -> Dead band on rising edge -> DBRED x 1/(TBCLK) = 1/(150Mhz)x100
                                //                           -> 666.66ns

    EPwm2Regs.DBCTL.bit.IN_MODE = 0;    //  0-> EPWMxA is source for both falling and rising edge
    EPwm2Regs.DBCTL.bit.POLSEL = 1;     //  1-> Active low complementry, A channel is inverted
                                        //  -> Although we didn't adjust AQCTLB, EPWMxB channel generate signal,
                                        //  -> Because we use channel A as source for channel B
    EPwm2Regs.DBCTL.bit.OUT_MODE = 3;   //  3-> Dead band is fully enabled for both rising and falling edge

    EALLOW;     //      -> All of trip-zone register, eallow protected
    EPwm2Regs.TZCTL.bit.TZA = 1;    //  -> EPWMxA high state when trip-zone event occur
    EPwm2Regs.TZCTL.bit.TZB = 1;    //  -> EPWMxB high state when trip-zone event occur

    EPwm2Regs.TZEINT.bit.OST = 0;   //  -> Disable one-shot interrupt generation
                                    //  -> We use ost event but don't want interrupt services
    EPwm2Regs.TZEINT.bit.CBC = 0;   //  -> Disable cycle-by-cycle interrupt generation
    EDIS;

/*
 *      -> Very first initial condition is in this format:
 *      Phase A up switch open, dowm switch close
 *      Phase B up switch close, dowm switch open
 *      Phase C up switch close, down switch open
 */
    EPwm2Regs.CMPA.half.CMPA = EPwm2Regs.TBPRD;   //  -> We can not wait Init_Zero_Condition function for pwm output
                                                  //  -> first duty cycle of EPWM2A is %0
                                                  //  -> first duty cycle of EPWM2b is %100

    EPwm3Regs.TBCTL.bit.FREE_SOFT = 11; //  11 -> ignore emulation suspend
    EPwm3Regs.TBCTL.bit.PHSDIR = 1;     //  1 -> Count up after the synchronization event
    EPwm3Regs.TBCTL.bit.CLKDIV = 0;     //  000 -> TBCLK = HSPCLK/1
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = 0;  //  -> HSPCLK = SYSCLKOUT/1
    EPwm3Regs.TBCTL.bit.SWFSYNC = 0;    //  0 -> no software sync produced
    EPwm3Regs.TBCTL.bit.SYNCOSEL = 0;   //  0 -> EPWMxSYNC synchronization output
    EPwm3Regs.TBCTL.bit.PRDLD = 0;      //  0 -> reload PRD on counter  when tbctr is equal to zero
    EPwm3Regs.TBCTL.bit.PHSEN = 1;      //  1 -> load the time-base counter with the phase register
                                                                               //        when an EPWMxSYNCI input signal occurs
    EPwm3Regs.TBCTL.bit.CTRMODE = 2;    //  2 -> up-down-count mode

    EPwm3Regs.TBPRD = 750;      // -> 750 = 1/2 x 150Mhz / 100kHz
                                // -> 750 -> 100 kHz pwm signal
                                // -> 3750 = 1/2 x 150MHz / 20kHz
                                //  -> 3750 -> 20 kHz pwm signal

                                //  -> !!! Attention !!! <-
                                //  -> If you change this you must also change DBRED, DBFED register

    EPwm3Regs.TBPHS.half.TBPHS = 500;   //      500 -> TBPRD = 750 and 2/3 240 degree phase shift
                                        //             with PHSDIR = 1

    EPwm3Regs.ETPS.bit.SOCBCNT = 0;     //  0 -> no event have occured
    EPwm3Regs.ETPS.bit.SOCBPRD = 0;     //  0 -> disable the SOCB event counter
    EPwm3Regs.ETPS.bit.SOCACNT = 0;     //  0 -> no event have occured
    EPwm3Regs.ETPS.bit.SOCAPRD = 0;     //  0 -> disable the SOCA event counter
    EPwm3Regs.ETPS.bit.INTCNT = 0;      //  0 -> no event have occured
    EPwm3Regs.ETPS.bit.INTPRD = 0;      //  0 -> disable the interrupt event counter

    EPwm3Regs.ETSEL.bit.SOCBEN = 0;     //  0 -> disable SOCB
    EPwm3Regs.ETSEL.bit.SOCBSEL = 0;    //  0 -> don't care
    EPwm3Regs.ETSEL.bit.SOCAEN = 0;     //  0 -> disable EPWMxSOCA
    EPwm3Regs.ETSEL.bit.SOCASEL = 0;    //  0 -> reserved
    EPwm3Regs.ETSEL.bit.INTEN = 0;      //  0 -> disable EPWMx_INT generation
    EPwm3Regs.ETSEL.bit.INTSEL = 0;     //  0 -> Reserved

    EPwm3Regs.AQCTLA.bit.CAU = 2;       //  2-> Pwm output high when counter is incrementing and
                                        //  counter equals the active CMPA register
    EPwm3Regs.AQCTLA.bit.CAD = 1;       //  1-> Pwm output low when counter is decrementing and
                                        //  -> counter equals the active CMPA register

    EPwm3Regs.DBRED = 50;      //  Dead band on rising edge -> DBRED x 1/(TBCLK) = 1/(150Mhz)x100
                                //                           -> 133.33ns
                                //  -> Dead band on rising edge -> DBRED x 1/(TBCLK) = 1/(150Mhz)x100
                                //                           -> 666.66ns
    EPwm3Regs.DBFED = 50;      //  Dead band on falling edge -> DBFED x 1/(TBCLK) = 1/(150Mhz)x100
                                //                            -> 133.33ns
                                //  -> Dead band on rising edge -> DBRED x 1/(TBCLK) = 1/(150Mhz)x100
                                //                           -> 666.66ns

    EPwm3Regs.DBCTL.bit.IN_MODE = 0;    //  0-> EPWMxA is source for both falling and rising edge
    EPwm3Regs.DBCTL.bit.POLSEL = 1;     //  1-> Active low complementry, A channel is inverted
                                        //  -> Although we didn't adjust AQCTLB, EPWMxB channel generate signal,
                                        //  -> Because we use channel A as source for channel B
    EPwm3Regs.DBCTL.bit.OUT_MODE = 3;   //  3-> Dead band is fully enabled for both rising and falling edge

    EALLOW;     //      -> All of trip-zone register, eallow protected
    EPwm3Regs.TZCTL.bit.TZA = 1;    //  -> EPWMxA high state when trip-zone event occur
    EPwm3Regs.TZCTL.bit.TZB = 1;    //  -> EPWMxB high state when trip-zone event occur

    EPwm3Regs.TZEINT.bit.OST = 0;   //  -> Disable one-shot interrupt generation
                                    //  -> We use ost event but don't want interrupt services
    EPwm3Regs.TZEINT.bit.CBC = 0;   //  -> Disable cycle-by-cycle interrupt generation
    EDIS;

/*
 *      -> Very first initial condition is in this format:
 *      Phase A up switch open, dowm switch close
 *      Phase B up switch close, dowm switch open
 *      Phase C up switch close, down switch open
*/
    EPwm3Regs.CMPA.half.CMPA = EPwm3Regs.TBPRD;   //  -> We can not wait Init_Zero_Condition function for pwm output
                                                  //  -> first duty cycle of EPWM3A is %0
                                                  //  -> first duty cycle of EPWM3b is %100
}

/*
 *      -> In this function we give initial value to global variables and struct with init func
 *      -> 0 is 0 any Q format but still we set 0 with Q transformation
 *      -> zero condition assignment
 *      -> Local variable initial value and
 *      -> variable initial value of struct with init func.
 */
void Init_Zero_Condition(void) {

/*
 *      -> Very first initial condition is in this format:
 *      Phase A up switch open, down switch close
 *      Phase B up switch close, down switch open
 *      Phase C up switch close, down switch open
 */
    DutyA_Q21_Swtch_Contrl = _IQ21(0.5);
    DutyB_Q21_Swtch_Contrl = _IQ21(0.5);
    DutyC_Q21_Swtch_Contrl = _IQ21(0.5);

    ANLG_PH_DC_Q19 = _IQ19(0.0);;
    ANLG_PH_DC_Avg_Q19 = _IQ19(0.0);

    Temp_Voltage_Q19 = _IQ19(0.0);       Normalize_Voltage_Q19 = _IQ19(0.0);

    Filter_Init_EMAVG(&var_DC_Avg_Q21_EMAVG);
    var_DC_Avg_Q21_EMAVG.Gain_a = _IQ21(0.00031406);

    Control_Init_PI_Controller(&var1_Q22_PI_Controller_Vltg_Contrl);
    var1_Q22_PI_Controller_Vltg_Contrl.Ki = _IQ22(0.1);
    var1_Q22_PI_Controller_Vltg_Contrl.Kp = _IQ22(0.8);
    var1_Q22_PI_Controller_Vltg_Contrl.Ki_Side_Constant = _IQ22mpy(var1_Q22_PI_Controller_Vltg_Contrl.Ki, _IQ22(0.00001));
    var1_Q22_PI_Controller_Vltg_Contrl.Output_Max = _IQ22(200.0);
    var1_Q22_PI_Controller_Vltg_Contrl.Output_Min = _IQ22(-200.0);


    EALLOW;
    EPwm1Regs.TZFRC.bit.OST = 0;
    EPwm2Regs.TZFRC.bit.OST = 0;
    EPwm3Regs.TZFRC.bit.OST = 0;
    EDIS;

    GpioDataRegs.GPACLEAR.bit.GPIO31 = 1;
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;

    interrupt_counter_test = 0;

}

/******************             *******************
 *
 *      -> Base interrupt trigger from PWM4
 *      -> All closed loop calculation realize in here
 *
 ******************             ******************/
interrupt void epwm4_base_isr(void) {
/*
 *      -> ADC assignment, ADC result register to variable
 *      -> Two conversion for each variable
 *      -> Temp values are average of two same channel
 *      -> ADC registers are 16 bit and the value which consist is 12 bit
 *      -> with (long) expression we transform 16 bit to 32 bit(actually it can not be necessary)
 */
    GpioDataRegs.GPBSET.bit.GPIO32 = 1;

    VDC_Voltage_Calculation_MACRO_Vltg_Clc((long) ((AdcMirror.ADCRESULT6 >> 1) + (AdcMirror.ADCRESULT14 >> 1)));
    ANLG_PH_DC_Q19 = Normalize_Voltage_Q19;

    var_DC_Avg_Q21_EMAVG.In = ANLG_PH_DC_Q19 << 2;
    Filter_MACRO_EMAVG(var_DC_Avg_Q21_EMAVG);
    ANLG_PH_DC_Avg_Q19 = var_DC_Avg_Q21_EMAVG.Out >> 2;

    if (TimerFlag)
    {
    var1_Q22_PI_Controller_Vltg_Contrl.Input = (DC_ref_Q19_Vltg_Contrl - ANLG_PH_DC_Avg_Q19) << 3;
    Control_MACRO_PI_Controller(var1_Q22_PI_Controller_Vltg_Contrl);
    }
    Control_Ref_Q21 = (var1_Q22_PI_Controller_Vltg_Contrl.Output) >> 1;

    DutyA_Q21_Swtch_Contrl = _IQ21mpy(Control_Ref_Q21 + Vp_Ref_Q21, One_Divided_Vpp_Ref_Q21);

    CMPA_Overflow_1_Swtch_Contrl = _IQsat(EPwm1Regs.TBPRD - _IQ21mpy(DutyA_Q21_Swtch_Contrl,EPwm1Regs.TBPRD), 745,5);
    EPwm1Regs.CMPA.half.CMPA = CMPA_Overflow_1_Swtch_Contrl;

    if(TimerFlag)
    {
        if(interrupt_counter_test %20000 == 0)
        {
            GpioDataRegs.GPATOGGLE.bit.GPIO31 = 1;
            GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1;
        }
    }
    interrupt_counter_test++;

    EPwm4Regs.ETCLR.bit.INT = 1;        //      -> Clear INT flag for this timer
    GpioDataRegs.GPBCLEAR.bit.GPIO32 = 1;

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;     //      -> Acknowledge this interrupt to receive more interrupts from group 3

}


__interrupt void cpu_timer0_isr(void)
{
    TimerFlag = 1;  //Turn on the flag to start PI operations
    // No acknowledge, interrupt fires only once.
    //Do not use any other interrupts of group 1.
}



