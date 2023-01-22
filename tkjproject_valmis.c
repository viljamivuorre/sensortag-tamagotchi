/*
 * JTKJ harjoitustyö syksy 2022
 * sensortag 'Tamagotchi' joka elää taustajärjestelmässä
 * 
 */

#include <sounds.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>

#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"
#include "buzzer.h"
#include <driverlib/aon_batmon.h>

#define STACKSIZE 2048
#define SMALLSTACKSIZE 512
#define MAXSIZE 15

Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char mpuTaskStack[STACKSIZE];
Char soundTaskStack[SMALLSTACKSIZE];
Char getDataTaskStack[STACKSIZE];

/*STATES*/

enum state {WAITING=0, READ_DATA, UPLOAD};
enum state programState = WAITING;

/*GLOBAL VARIABLES*/

double ambientLight = 100;

float accx[MAXSIZE]={0};
float accy[MAXSIZE]={0};
float accz[MAXSIZE]={0};
float gyrox[MAXSIZE]={0};
float gyroy[MAXSIZE]={0};
float gyroz[MAXSIZE]={0};

float ax;
float ay;
float az;
float gx;
float gy;
float gz;

uint8_t uartBuffer[16];
char uartStr[80];

uint8_t timeCounter = 0;
uint8_t index = 0;

uint8_t eat = 0;
uint8_t pet = 0;
uint8_t exercise = 0;

static PIN_Handle buttonHandle;             //LEFT BUTTON
static PIN_State buttonState;

static PIN_Handle ledHandle;
static PIN_State ledState;

static PIN_Handle mpuHandle;    //ACCELOMETER AND GYROSCOPE
static PIN_State mpuState;


PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};
PIN_Config ledConfig[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};
static PIN_Config mpuConfig[] = {
   Board_MPU_POWER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};

static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

void ButtonFxn(PIN_Handle handle, PIN_Id pinId) {                                                                                                                                                                                                                                                                                                                                                                   // dont press the button :---D

    if (PIN_getOutputValue( Board_LED1 ) == 0) {

        PIN_setOutputValue(ledHandle, Board_LED0, 1);

    } else {

        PIN_setOutputValue(ledHandle, Board_LED0, 0);
    }
}

//Pet when covered
int analysePet(){
    if(ambientLight<5){
        return 1;
    }
    return 0;
}

//3 up and down movements
int analyseExcercise(){

    uint8_t i;
    uint8_t count = 0;
    for(i = 0; i < MAXSIZE; i++){
        
        if(accz[i] > 3 || accz[i] < -3){
           count++;
        }
        if(count >= 2){
            return 1;
        }
    }
    return 0;
}

//left side rotate upside down and back
int analyseEat(){
    
    uint8_t i;
    for(i = 0; i < MAXSIZE; i++){

        float x = gyrox[i];

        if(x > 200 || x < -200){
            return 1;
        }
    }
    return 0;
}
//right side rotate upside down and back
int analyseActivate(){
    
    uint8_t i;
    for(i = 0; i < MAXSIZE; i++){

        float y = gyroy[i];

        if(y > 200 || y < -200){
            return 1;
    
        }
    }
    return 0;
}


void getDataFxn(UArg arg0, UArg arg1){
    while (1) {
        if (programState == READ_DATA) {
            if (analyseEat() == 1 || analyseExcercise() == 1 || analysePet() == 1 || analyseActivate() == 1) {
                if (analyseEat() == 1) {
                    eat++;
                    soundState = ONEBEEP;
                }
                if (analyseExcercise() == 1) {
                    exercise++;
                    soundState = ONEBEEP;
                }
                if (analysePet() == 1) {
                    pet++;
                    soundState = ONEBEEP;
                }
                if (analyseActivate() == 1) {
                    eat++;
                    exercise++;
                    pet++;
                    soundState = ONEBEEP;
                }

            }
            ambientLight = 100;

            programState = UPLOAD;
            Task_sleep(100000 / Clock_tickPeriod);
        } else {
        Task_sleep(100000 / Clock_tickPeriod);
        }
    }
}

int validMSG(char *msg) {                                                                   //VALID MSG CHECK
    if (msg[0] == '2' && msg[1] == '2' && msg[2] == '5' && msg[3] == '7') {

        if (strstr(msg, "Severe") || strstr(msg, "scratch") || strstr(msg, "Running low")) {
            return 1;
        } else if (strstr(msg,"Too late")){
            return 2;
        }
        return 0;
    }
    return 0;
}

void uartFxn(UART_Handle handle, void *rxBuf, size_t len){                            //RECEIVE MESSAGE
        if (validMSG(rxBuf) >= 1) {
            strncat(uartStr, rxBuf, 80);
        }
    UART_read(handle, rxBuf, 80);
}

void sendMSG(UART_Handle handle, char *msg, int length) {                              //SEND MESSAGE
        UART_write(handle, msg, length++);
}

Void uartTaskFxn(UArg arg0, UArg arg1) {
    UART_Handle uart;
    UART_Params uartParams;

    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readMode = UART_MODE_CALLBACK;
    uartParams.readCallback = &uartFxn;
    uartParams.baudRate = 9600; // SPEED 9600baud
    uartParams.dataLength = UART_LEN_8; // 8
    uartParams.parityType = UART_PAR_NONE; // n
    uartParams.stopBits = UART_STOP_ONE; // 1


    uart = UART_open(Board_UART, &uartParams);

    if(uart == NULL){
        System_abort("Error opening the UART");
    }

    UART_read(uart, uartBuffer, 80);


    while (1) {
        if (programState == UPLOAD) {

            if (eat==1 && exercise==0 && pet==0) {
                char feedMSG[] = "id:2257,EAT:1,ping";
                sendMSG(uart, feedMSG, strlen(feedMSG) + 1);
            }else if (eat==0 && exercise==1 && pet==0) {
                char exerciseMSG[] = "id:2257,EXERCISE:1,ping";
                sendMSG(uart, exerciseMSG, strlen(exerciseMSG) + 1);
            }else if (eat==0 && exercise==0 && pet==1) {
                char petMSG[] = "id:2257,PET:1,ping";
                sendMSG(uart, petMSG, strlen(petMSG) + 1);
            } else if  (eat + exercise + pet >= 2) {
                char activateMSG[22];
                sprintf(activateMSG, "id:2257,ACTIVATE:%hu;%hu;%hu", eat, exercise, pet);
                sendMSG(uart, activateMSG, 23);
            }

            eat = 0;
            exercise = 0;
            pet = 0;
            programState = WAITING;


           if (validMSG(uartStr) == 1) {
               soundState = TWOBEEP;
               System_printf(uartStr);
               System_printf("\n");
           } else if (validMSG(uartStr) == 2) {
               soundState = MYSTICALTREE;
               System_printf(uartStr);
               System_printf("\n");
           }

            memset(uartStr,0,80);
        }

        Task_sleep(150000 / Clock_tickPeriod);
    }
}
Void sensorTaskFxn(UArg arg0, UArg arg1) {

    I2C_Handle i2c;            //OPT GATEWAY
    I2C_Params i2cParams;

    I2C_Handle i2cMPU;         //MPU GATEWAY
    I2C_Params i2cMPUParams;


    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    Task_sleep(100000/ Clock_tickPeriod);

    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL){
        System_abort("Error Initializing I2CMPU\n");
    }
    mpu9250_setup(&i2cMPU);
    I2C_close(i2cMPU);

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    i2c = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    Task_sleep(100000 / Clock_tickPeriod);
    opt3001_setup(&i2c);
    I2C_close(i2c);

    while (1) {
        if (programState == WAITING){
            if (timeCounter != 0 && timeCounter%15 == 0) {
                i2c = I2C_open(Board_I2C_TMP, &i2cParams);
                int light = opt3001_get_data(&i2c);
                if (light>=0){
                    ambientLight=light;
                }
                I2C_close(i2c); 
            } else {
                i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
                mpu9250_get_data(&i2cMPU, &accx[index], &accy[index], &accz[index], &gyrox[index], &gyroy[index], &gyroz[index]);
                I2C_close(i2cMPU);
                index++;
            }
            if (timeCounter != 0 && timeCounter%15 == 0) {
                programState = READ_DATA;
                timeCounter = 0;
                index = 0;
                Task_sleep(15000 / Clock_tickPeriod);
            } else {
                timeCounter++;
                Task_sleep(150000 / Clock_tickPeriod);
            }
        } else {
            Task_sleep(15000 / Clock_tickPeriod);
        } 
    }
}
int main(void) {
    
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;

    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;

    Task_Handle getDataTaskHandle;
    Task_Params getDataTaskParams;

    Task_Handle soundTaskHandle;
    Task_Params soundTaskParams;

    

    Board_initGeneral();
    Init6LoWPAN();
    Board_initUART();
    Board_initI2C();

    mpuHandle = PIN_open(&mpuState, mpuConfig);
    if (mpuHandle == NULL) {
        System_abort("Pin open failed!");
    }

    buttonHandle = PIN_open(&buttonState, buttonConfig);
    if(!buttonHandle) {
        System_abort("Error initializing button pins\n");
    }

    ledHandle = PIN_open(&ledState, ledConfig);
    if(!ledHandle) {
        System_abort("Error initializing LED pins\n");
    }

    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    Task_Params_init(&getDataTaskParams);
    getDataTaskParams.stackSize = STACKSIZE;
    getDataTaskParams.stack = &getDataTaskStack;
    getDataTaskParams.priority = 2;
    getDataTaskHandle = Task_create(getDataFxn, &getDataTaskParams, NULL);
    if (getDataTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=1;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    Task_Params_init(&soundTaskParams);
    soundTaskParams.stackSize = SMALLSTACKSIZE;
    soundTaskParams.stack = &soundTaskStack;
    soundTaskParams.priority = 2;
    soundTaskHandle = Task_create(soundTask, &soundTaskParams, NULL);
    if (soundTaskHandle == NULL) {
        System_abort("Task create failed");
    }
    System_printf("HEI maailma!\n");
    BIOS_start();

    return (0);
}
