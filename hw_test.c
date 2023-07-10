#include "hw_test.h"
#if ENABLE_TEST_MODE
#    include "main.h"
#    include "nb.h"
#    include "irda.h"
#    include "key.h"
#    include "led.h"
#    include "stdio.h"
#    include "system_timer.h"
#    include "key.h"

/* 使用模组 */
#    if defined(__UMN601)
#        include "UMN601.h"
#    elif defined(__MN316)
#        include "MN316.h"
#    endif

/* 测试类型，以下不能同时测试，单次只能测试一项 */

#    if (CONNECT_TYPE == USE_UDP)
// 目标IP信息
uint8_t ip_str[] = "218.75.104.131"; // 恒生公网IP 218.75.104.131
uint8_t port_str[] = "18080";        // 李立 18080;李少宾 8383
#    elif (CONNECT_TYPE == USE_COAP)
uint8_t ip_str[] = "117.60.157.137";
uint8_t port_str[] = "5683";
#    endif

/* 定义使用红外打印；由于红外波特率较低，若测试和时序有关的项目时，要考虑是否会有影响 */
#    ifndef IRDA_PRINTF_ENABLE
#        define printf(...) (void)0
#    endif

int fputc(int ch, FILE *f)
{
#    ifdef IRDA_PRINTF_ENABLE
    uint8_t c = (uint8_t)ch;
    Sys_Irda_Send(&c, 1);
#    endif
    return ch;
}

struct testInfTypedef {
    uint16_t testCnt;
    uint16_t cPinSuccessCnt;
    uint16_t cPinFailedCnt;
    uint32_t avgDurTimeMs;
    uint32_t totalTimeMs;
} testInf;

#    if defined(__UMN601)
extern bool Ap_Nb_Reset(void);
extern bool Ap_At_Cscon(void);
extern bool Ap_Send_At_Cmd_Keywords(char *string, char *keywords, uint32_t outtime, uint8_t times);
extern bool Ap_At_Cmd_Wait_Keywords(char *keywords, uint32_t outtime);
extern void Sys_NbPwr_State(uint8_t status);
extern void Ap_Nb_Get_Clk(void);
#    elif defined(__MN316)
extern bool Ap_Nb_Open(void);
extern bool Ap_At_Cscon(void);
extern bool Ap_Send_At_Cmd_Keywords(char *string, char *keywords, uint32_t outtime, uint8_t times);
extern bool Ap_At_Cmd_Wait_Keywords(char *keywords, uint32_t outtime);
extern void Ap_Nb_Read_State(void);
extern void Ap_Nb_Get_Clk(void);
#    endif

static void HwSimTestInit(void);
static bool HwReadSimTestFunc(void);
static void HwTestNbResetAndPowerOn(void);
static void HwTestNbPowerDown(void);
static bool HwTestAp_At_Synchronization(void);
static bool HwTestAp_At_Cpin(void);
static bool HwTestAp_At_Cscon(void);
static bool HwTestAp_Nb_Trs_Frame(uint8_t *ptr, uint16_t len);
static bool HwTestAp_Udp_Trs_Frame(uint8_t *ptr, uint16_t len);
static bool HwTestAp_Coap_Trs_Frame(uint8_t *ptr, uint16_t len);
static bool HwTestAp_Aep_Coap_Rev_Frame(void);


uint8_t testSendData[TEST_SEND_DATA_LEN] = {
    0xA9, 0x9A, 0x00, 0x00, 0x01, 0x01, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAF, 0xFF, 0x08, 0x00, 0x00,
    0x15, 0x02, 0x00, 0x66, 0x00, 0x64, 0x00, 0x01, 0x23, 0x00, 0x00, 0x38, 0x36, 0x34, 0x39, 0x32, 0x32, 0x30, 0x36,
    0x31, 0x38, 0x31, 0x38, 0x33, 0x30, 0x38, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
    0x20, 0x23, 0x06, 0x27, 0x09, 0x33, 0x50, 0x02, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x04, 0x20, 0x23, 0x06, 0x27, 0x09, 0x33, 0x50, 0x0A, 0x68, 0x01, 0x05, 0x12, 0x00, 0x00, 0x20, 0x23, 0x06,
    0x27, 0x09, 0x30, 0x00, 0x01, 0x1E, 0x00, 0x02, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x0E, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x0A, 0xB0, 0x00, 0x0B, 0xD3, 0x5E, 0xB6, 0x09, 0x3C, 0xC4, 0x16
};

void HwTest(void)
{
    bool status = false;
    uint32_t startTick;
    testInf.totalTimeMs = 0;
    testInf.testCnt = 0;

    for (size_t i = 0; i < 100; i++) {
        testSendData[i] = i;
    }
    testSendData[0] = 0xa9;

    HwSimTestInit();

    while (1) {
        // 上电&复位模块
        Sys_NbPwr_State(true);
        HwTestNbResetAndPowerOn();

        startTick = _sysOptParams._sysTick;

        // 读取SIM信息测试
        status = HwReadSimTestFunc();
#    if (TEST_MODE == READ_SIM_INF_TEST)
        goto SHUTDOWN_AND_WAIT;
#    elif (TEST_MODE == CSCON_TEST)
        if (status == false) {
            goto SHUTDOWN_AND_WAIT;
        }
        startTick = _sysOptParams._sysTick;
#    endif

        // 驻网测试
        status = HwTestAp_At_Cscon();
#    if (TEST_MODE == CSCON_TEST)
        goto SHUTDOWN_AND_WAIT;
#    elif (TEST_MODE == NB_TRANSMIT_TEST)
        if (status == false) {
            goto SHUTDOWN_AND_WAIT;
        }
        startTick = _sysOptParams._sysTick;
#    endif

        // 数据发送测试
        status = HwTestAp_Nb_Trs_Frame(testSendData, TEST_SEND_DATA_LEN);
#    if (TEST_MODE == NB_TRANSMIT_TEST)
        goto SHUTDOWN_AND_WAIT;
#    elif (TEST_MODE == MORE_TEST)
        if (status == false) {
            goto SHUTDOWN_AND_WAIT;
        }
        startTick = _sysOptParams._sysTick;
#    endif

        ///< 关闭NB模组电源并等待3s
    SHUTDOWN_AND_WAIT:
        ///< 统计打印结果
        if (status == false) {
            testInf.cPinFailedCnt++;
        } else {
            testInf.cPinSuccessCnt++;
            testInf.totalTimeMs += _sysOptParams._sysTick - startTick;
            testInf.testCnt++;
            testInf.avgDurTimeMs = testInf.totalTimeMs / testInf.cPinSuccessCnt;
        }
        HwTestNbPowerDown();
        printf("t:%d,f:%d,avg %dms\r\n", testInf.cPinSuccessCnt + testInf.cPinFailedCnt, testInf.cPinFailedCnt,
               testInf.avgDurTimeMs);
    }
}

void HwSimTestInit(void)
{

    Sys_SysTick_Init(1000);
    memset((uint8_t *)&_sysOptParams, 0, sizeof(_sysOptParams));
    _baseParams.BaudRate = 2400; // 红外通讯波特率2400pbs
    Sys_Timer0_Init();           // 系统工作时钟初始化

    Sys_NoUsePort_Init();        // 未使用引脚初始化
    Sys_Irda_Port_Init();
    Sys_Irda_Switch(true);       // 开启红外电源
    Sys_Led_Init();              // 指示灯初始化
    Sys_Key_Init();              // 磁开关初始化
    Sys_Led_State(false);        // 开启指示灯
    Sys_Rtc_Init();              // RTC时钟初始化
    Sys_Nb_Uart_Init();          // NB串口初始化
}

static bool HwReadSimTestFunc(void)
{
    bool status;

    // 读取模组及SIM信息测试
    status = HwTestAp_At_Synchronization(); // 同步
    if (status == false) {
        return false;
    }

    status = HwTestAp_At_Cpin(); // 获取NB模组和SIM信息
    if (status == false) {
        return false;
    }

    return true;
}

static void HwTestNbResetAndPowerOn(void)
{
    ///< NB模组上电复位
#    if defined(__UMN601)
    ///< UMN601模组中有复位操作，复位后在掉电，再重新上电
    Ap_Nb_Reset();                                    // NB模组复位

    Gpio_SetIO(NB_PWR_CTR_GPIO_Port, NB_PWR_CTR_Pin); // 低电平
    Sys_Wdt_Feed();                                   // 喂狗
    Sys_DelayMs(200);
    Gpio_ClrIO(NB_PWR_CTR_GPIO_Port, NB_PWR_CTR_Pin); // 高电平
    Sys_DelayMs(50);
    Gpio_SetIO(NB_PWR_CTR_GPIO_Port, NB_PWR_CTR_Pin); // 低电平
    Sys_DelayMs(50);
    Sys_Wdt_Feed();
#    elif defined(__MN316)
    Ap_Nb_Open();
#    endif
}

static void HwTestNbPowerDown(void)
{
#    if defined(__MN316)
    // 验频点的记录及清除需要进行 flash cache 的刷新，因此MN316模组掉电前要先发送关机指令
    Ap_Send_At_Cmd_Keywords("AT+CPOF=0\r", "OK", 400, 3);
#    endif
    Gpio_SetIO(NB_PWR_CTR_GPIO_Port, NB_PWR_CTR_Pin);
    Sys_DelayMs(3000);
}

static bool HwTestAp_At_Synchronization(void)
{
    bool status;
#    if defined(__UMN601)

    status = Ap_Send_At_Cmd_Keywords("AT\r", "OK", 400, 20);
    if (status == false) {
        return false;
    }

    // 驻网失败则删除优选频点
    status = Ap_Send_At_Cmd_Keywords("AT+CFUN=0\r", "OK", 400, 1);
    if (status == false) {
        return false;
    }
    status = Ap_Send_At_Cmd_Keywords("AT+ECFREQ=3\r", "OK", 5000, 1);
    if (status == false) {
        return false;
    }

    status = Ap_Send_At_Cmd_Keywords("AT+CFUN=1\r", "OK", 400, 1);
    if (status == false) {
        return false;
    }
#    elif defined(__MN316)
    status = Ap_Send_At_Cmd_Keywords("AT\r", "OK", 400, 20);
    if (status == false) {
        return false;
    }

    //    // 清除存储的频点，测试316模组在地下室效果不好，添加清除存储频点的步骤
    //    status = Ap_Send_At_Cmd_Keywords("AT+CFUN=0\r", "OK", 10000, 1);
    //    if (status == false) {
    //        return false;
    //    }
    //    status = Ap_Send_At_Cmd_Keywords("AT+NCSEARFCN\r", "OK", 5000, 2);
    //    if (status == false) {
    //        return false;
    //    }

    // 启动模块
    status = Ap_Send_At_Cmd_Keywords("AT+CFUN=1\r", "OK", 400, 5);
    if (status == false) {
        return false;
    }
#    endif

    return true;
}

static bool HwTestAp_At_Cpin(void)
{
    bool status;

#    if defined(__UMN601)
    ///< status |= Ap_At_Cpin();
    status = Ap_Send_At_Cmd_Keywords("AT+UMV\r", "220414", 400, 1);
    if (status == true) {
        status = Ap_Send_At_Cmd_Keywords("AT+RICFG=0\r", "OK", 400, 1);
        if (status == false) {
            return false;
        }
    }

    // 获取版本号20220727 针对错误版本号220414和220719 关闭RICFG
    status = Ap_Send_At_Cmd_Keywords("AT+UMV\r", "220719", 400, 1);
    if (status == true) {
        status = Ap_Send_At_Cmd_Keywords("AT+RICFG=0\r", "OK", 400, 1);
        if (status == false) {
            return false;
        }
    }

    status = Ap_Send_At_Cmd_Keywords("AT+CGSN=1\r", "+CGSN:", 400, 5);
    if (status == false) {
        return false;
    }

    // get imsi
    status = Ap_Send_At_Cmd_Keywords("AT+CIMI=?\r", "OK", 400, 5);
    if (status == false) {
        return false;
    }

    status = Ap_Send_At_Cmd_Keywords("AT+CIMI\r", "OK", 400, 1);
    if (status == false) {
        return false;
    }

    status = Ap_Send_At_Cmd_Keywords("AT+ECICCID\r", "+ECICCID:", 400, 5);
    if (status == false) {
        return false;
    }

    status = Ap_Send_At_Cmd_Keywords("AT+CIMI\r", "OK", 400, 5);
    if (status == false) {
        return false;
    }


#    elif defined(__MN316)
    status = Ap_Send_At_Cmd_Keywords("AT+CGSN=1\r", "+CGSN:", 400, 5);
    if (status == false) {
        return false;
    }
    status = Ap_Send_At_Cmd_Keywords("AT+NCCID\r", "+NCCID:", 400, 5);
    if (status == false) {
        return false;
    }
    status = Ap_Send_At_Cmd_Keywords("AT+CIMI\r", "OK", 400, 5);
    if (status == false) {
        return false;
    }
//    status = Ap_Send_At_Cmd_Keywords("AT+CIMI\r", "OK", 400, 5);
//    printf("line%d %d\r\n", __LINE__, status);
//    errCnt += !status;
#    endif
    return true;
}

static bool HwTestAp_At_Cscon(void)
{
    uint8_t loop = 60;
#    if defined(__UMN601)
    while (loop--) {
        if (Ap_Send_At_Cmd_Keywords("AT+CEREG?\r", "OK", 400, 1)) {
            if ((strstr((const char *)_uartRxTx.rBuf, ",1") != NULL) ||
                (strstr((const char *)_uartRxTx.rBuf, ",5") != NULL)) {
                return true;
            }
        }
        Sys_DelayMs(2000);

        Sys_Wdt_Feed(); // 喂狗
    }


    return false;
#    elif defined(__MN316)


    if (Ap_Send_At_Cmd_Keywords("AT+CGATT=1\r", "OK", 400, 5) == false) // 启动注网
    {
        return false;
    }
    while (loop--) {
        if (Ap_Send_At_Cmd_Keywords("AT+CGATT?\r", "OK", 400, 2)) // 查询注网
        {
            if (strstr((const char *)_uartRxTx.rBuf, "+CGATT:1") != NULL) {
                return true;
            }
        }
        Sys_DelayMs(2000);

        Sys_Wdt_Feed(); // 喂狗
    }
    return false;
#    endif
}

static bool HwTestAp_Nb_Trs_Frame(uint8_t *ptr, uint16_t len)
{
#    if (CONNECT_TYPE == USE_UDP)
    ///< 设备网络工作状态参数更新
    Ap_Nb_Read_State();

    //    ///< 设备系统时间同步，同步基站时间
    Ap_Nb_Get_Clk();
    return HwTestAp_Udp_Trs_Frame(ptr, len);
#    elif (CONNECT_TYPE == USE_COAP)
    ///< 设备网络工作状态参数更新
    Ap_Nb_Read_State();

    //    ///< 设备系统时间同步，同步基站时间
    Ap_Nb_Get_Clk();
    return HwTestAp_Coap_Trs_Frame(ptr, len);
#    endif
}

static bool HwTestAp_Udp_Trs_Frame(uint8_t *ptr, uint16_t len)
{
    static uint8_t status = 0;
    static uint8_t commandStr[64];
    uint8_t date8;
    int cmdLength = 0;

#    if defined(__UMN601)
    ///< 设备网络工作状态参数更新
    Ap_Nb_Read_State();

    //    ///< 设备系统时间同步，同步基站时间
    Ap_Nb_Get_Clk();

    status = Ap_Send_At_Cmd_Keywords("AT+SKTCREATE=1,2,17\r", "OK", 10000, 1);
    if (status == false) {
        return false;
    }

    (void)sprintf((char *)commandStr, "AT+SKTCONNECT=1,%s,%s\r", ip_str, port_str);
    status = Ap_Send_At_Cmd_Keywords((char *)commandStr, "OK", 10000, 1);
    //    Ap_Send_At_Cmd_Keywords("AT+SKTCONNECT=1,218.75.104.131,18080\r", "OK", 10000, 1);

    cmdLength = sprintf((char *)commandStr, "AT+SKTSEND=1,%d,", len);
    Sys_Nb_Send(commandStr, cmdLength);
    for (uint16_t i = 0; i < len; i++) {
        date8 = (*(ptr + i) >> 4);
        date8 = (date8 > 0x09) ? (date8 - 0x0A + 0x41) : (date8 + 0x30);
        Sys_Nb_Send(&date8, 1);

        date8 = (*(ptr + i) & 0x0F);
        date8 = (date8 > 0x09) ? (date8 - 0x0A + 0x41) : (date8 + 0x30);
        Sys_Nb_Send(&date8, 1);
    }
    date8 = '\r';
    Sys_Nb_Send(&date8, 1);
    // wait for ok
    status = Ap_At_Cmd_Wait_Keywords("OK", 1500);
    if (status == false) {
        return false;
    }

    status = Ap_At_Cmd_Wait_Keywords("+SKTRECV: 1,100,", 3000); // 收到的包长度为100
    if (status == false) {
        return false;
    }

    Sys_Nb_Rst_Rx();

#    elif defined(__MN316)

    // 获取NB网络信息
    Ap_Nb_Read_State();
    status = Ap_Send_At_Cmd_Keywords("AT+NUESTATS=CELL\r", "OK", 400, 2);
    if (status == false) {
        return false;
    }

    // 获取基站时间
    Ap_Nb_Get_Clk();
    status = Ap_Send_At_Cmd_Keywords("AT+NSOCR=DGRAM,17,6666,1,1\r", "OK", 1000, 2);
    if (status == false) {
        return false;
    }

    cmdLength = sprintf((char *)commandStr, "AT+NSOST=0,%s,%s,%d,", ip_str, port_str, len);
    //    cmdLength = sprintf((char *)commandStr, "AT+NSOST=0,218.75.104.131,18080,%d,", len);
    Sys_Nb_Send(commandStr, cmdLength); // 发送指令
    for (size_t i = 0; i < len; i++) {
        date8 = (*(ptr + i) >> 4);
        date8 = (date8 > 0x09) ? (date8 - 0x0A + 0x41) : (date8 + 0x30);
        Sys_Nb_Send(&date8, 1);

        date8 = (*(ptr + i) & 0x0F);
        date8 = (date8 > 0x09) ? (date8 - 0x0A + 0x41) : (date8 + 0x30);
        Sys_Nb_Send(&date8, 1);
    }
    date8 = '\r';
    Sys_Nb_Send(&date8, 1);

    // wait for ok
    status = Ap_At_Cmd_Wait_Keywords("OK", 1500);
    if (status == false) {
        return false;
    }


    // status = Ap_Send_At_Cmd_Keywords("AT+NSORF=0,512\r", "+NSORF:0,218.75.104.131,18080,100", 4000, 9);
    cmdLength = sprintf((char *)commandStr, "+NSORF:0,%s,%s,%d", ip_str, port_str, TEST_SEND_DATA_LEN);
    status = Ap_Send_At_Cmd_Keywords("AT+NSORF=0,512\r", (char *)commandStr, 4000, 9);
    if (status == false) {
        return false;
    }

    Sys_Nb_Rst_Rx();
#    endif

    return true;
}

static bool HwTestAp_Coap_Trs_Frame(uint8_t *ptr, uint16_t len)
{
    uint8_t status;
    uint8_t date8;
    static uint8_t commandStr[64];

#    if defined(__UMN601)

    /* Ap_Aep_Coap_Bulid_Link */
    if (Ap_Send_At_Cmd_Keywords("AT+CTM2MSETNNMIMOD=2\r", "OK", 1000, 2) == false) {
        return false;
    }

    uint16_t index = snprintf((char *)commandStr, sizeof(commandStr), "AT+CTM2MSETPM=%s,%s,38400\r", ip_str, port_str);
    status = Ap_Send_At_Cmd_Keywords((char *)commandStr, "OK", 20000, 2);
    if (status == false) {
        return false;
    }

    status = Ap_Send_At_Cmd_Keywords("AT+CTM2MREG\r", "+CTM2M: reg,0", 20000, 1);
    if (status == false) {
        return false;
    }

    status = Ap_At_Cmd_Wait_Keywords("+CTM2M: obsrv,0", 6000);
    if (status == false) {
        return false;
    }

    /* Ap_Aep_Coap_Trs_Frame */
    date8 = snprintf((char *)commandStr, sizeof(commandStr), "AT+CTM2MSEND=");
    Sys_Nb_Send(commandStr, date8);
    for (uint16_t i = 0; i < len; i++) {
        date8 = (*(ptr + i) >> 4);
        date8 = (date8 > 0x09) ? (date8 - 0x0A + 0x41) : (date8 + 0x30);
        Sys_Nb_Send(&date8, 1);

        date8 = (*(ptr + i) & 0x0F);
        date8 = (date8 > 0x09) ? (date8 - 0x0A + 0x41) : (date8 + 0x30);
        Sys_Nb_Send(&date8, 1);
    }
    date8 = '\r';
    Sys_Nb_Send(&date8, 1);
    status = Ap_At_Cmd_Wait_Keywords("+CTM2M: send,", 1200);
    if (status == false)
        return false;
    status = HwTestAp_Aep_Coap_Rev_Frame();
    if (status == false)
        return false;

    return true;
#    elif defined(__MN316)

#    endif
}

static bool HwTestAp_Aep_Coap_Rev_Frame(void)
{
    uint8_t status;

    status = Ap_Send_At_Cmd_Keywords("AT+CTM2MNMGR?\r", "stored_num: 0", 5000, 1);
    if (status == false) {
        return false;
    }

    status = Ap_Send_At_Cmd_Keywords("AT+CTM2MNMGR\r", "OK", 5000, 1);
    if (status == false) {
        return false;
    }

    return true;
}
#endif

