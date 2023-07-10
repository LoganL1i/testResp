#ifndef __HW_TEST_H__
#define __HW_TEST_H__


//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------
//   <e>Enable test mode
//   <i>Enable test mode
#define ENABLE_TEST_MODE 0

//     <o>connect type
//        <0=> UDP
//        <1=> TCP
//        <2=> COAP
//        <3=> oneNet
//     <i>Selects test mode
#define CONNECT_TYPE 2

//     <o>Test mode
//        <0=> Read SIM and modle inf test
//        <1=> CSCON test
//        <2=> Transmit test
//     <i>Selects test mode
#define TEST_MODE 2

//     <c1>Enable irda printf
//     <i>Enable irda printf
#define IRDA_PRINTF_ENABLE
//     </c>

//     <o>Set transmit data length
//     <i>Set transmit data length
//     <1-512>
#define TEST_SEND_DATA_LEN 129

//   </e>
//------------- <<< end of configuration section >>> ---------------------------


#if ENABLE_TEST_MODE

#    define READ_SIM_INF_TEST 0
#    define CSCON_TEST        1
#    define NB_TRANSMIT_TEST  2


#    define USE_UDP           0
#    define USE_TCP           1
#    define USE_COAP          2
#    define USE_ONENET        3

void HwTest(void);

#endif

#endif
