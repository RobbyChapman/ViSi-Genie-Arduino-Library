/**
 * Created by Robby Chapman on 05/01/17. A quick test application for VisiGenieSerial
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/hibernate.h"
#include "visiGenieSerial.h"

#define EXT_CRYSTAL    0
#define CLK_CFG        (SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480)
#define CLK_MHZ        120000000
#define UART_BASE      UART3_BASE
#define UART_RX        GPIO_PA4_U3RX
#define UART_TX        GPIO_PA5_U3TX
#define UART_BASE_INT  INT_UART3
#define UART_CFG       (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE)
#define UART_BAUD      115200
#define UART_PIN_CFG   GPIO_PIN_4 | GPIO_PIN_5
#define MILLI_PER_SEC  1000

/* Keep clock global */
uint32_t g_ui32SysClock;

/* init */
static void initUart(void);
static void initRtc(void);
static void initGpio(void);
static void initDisplayAnimationLoop(void);

/* UserApiConfig */
static bool uartAvailHandler(void);
static uint8_t uartReadHandler(void);
static void uartWriteHandler(uint32_t val);
static uint32_t uartGetMillis(void);
static void resetDisplay(void);

/* Event handlers */
static void myGenieEventHandler(void);
static void HibernateHandler(void);

int main(void) {

  initGpio();
  initRtc();
  initUart();
  initDisplayAnimationLoop();
}

static void initDisplayAnimationLoop(void) {
  
   int32_t waitPeriod = 0, gaugeAddVal = 1, gaugeVal = 0;
    static UserApiConfig userConfig = {
    .available = uartAvailHandler,
    .read =  uartReadHandler,
    .write = uartWriteHandler,
    .millis = uartGetMillis
  };

  genieInitWithConfig(&userConfig);
  genieAttachEventHandler(myGenieEventHandler);
  resetDisplay();
  genieWriteContrast(15); 
  genieWriteStr(0, GENIE_VERSION);
  ROM_SysCtlDelay((g_ui32SysClock / 3) * 1);
  /* Animate gauges */
  for(;;) {
      waitPeriod = uartGetMillis();
      ROM_SysCtlDelay((g_ui32SysClock / 18) * 1);
      genieDoEvents(true);
      genieWriteObject(GENIE_OBJ_COOL_GAUGE, 0, gaugeVal);
      genieWriteObject(GENIE_OBJ_LED_DIGITS, 0, gaugeVal);
      gaugeVal += gaugeAddVal;
      if (gaugeVal == 99) gaugeAddVal = -1;
      if (gaugeVal == 0) gaugeAddVal = 1;
  }
}

static void initGpio(void) {
  
  g_ui32SysClock = MAP_SysCtlClockFreqSet(CLK_CFG, CLK_MHZ);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
  ROM_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
  ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_7);
  ROM_IntMasterEnable();
}

static void initRtc(void) {
  
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
  ROM_HibernateEnableExpClk(g_ui32SysClock);
  ROM_HibernateClockConfig(HIBERNATE_OSC_HIGHDRIVE);
  ROM_HibernateRTCSet(0);
  ROM_HibernateRTCEnable();
  HibernateRTCMatchSet(0,HibernateRTCGet()+1);
  HibernateIntRegister(HibernateHandler);
  HibernateIntEnable(HIBERNATE_INT_RTC_MATCH_0);
  HibernateCounterMode(HIBERNATE_COUNTER_RTC);
}

static void initUart(void) {
  
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  GPIOPinConfigure(UART_RX);
  GPIOPinConfigure(UART_TX);
  ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, UART_PIN_CFG);
  ROM_UARTConfigSetExpClk(UART_BASE, g_ui32SysClock, UART_BAUD, UART_CFG);
}

static void HibernateHandler(void) {

  uint32_t ui32Status = HibernateIntStatus(1);
  HibernateIntClear(ui32Status);
  HibernateRTCMatchSet(0,ROM_HibernateRTCGet()+1);  
}

static void resetDisplay(void) {

  GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, 0x0);
  ROM_SysCtlDelay((g_ui32SysClock / 3));
  GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_PIN_7 );
  ROM_SysCtlDelay((g_ui32SysClock / 3) * 5);
}

static void myGenieEventHandler(void) {
  
  GenieFrame Event;
  genieDequeueEvent(&Event);
  int32_t slider_val = 0;
  
  if (Event.reportObject.cmd == GENIE_REPORT_EVENT) {
    if (Event.reportObject.object == GENIE_OBJ_SLIDER){
      if (Event.reportObject.index == 0){
        slider_val = genieGetEventData(&Event);
        genieWriteObject(GENIE_OBJ_LED_DIGITS, 0, slider_val);
      }
    }
  }
  if (Event.reportObject.cmd == GENIE_REPORT_OBJ) {
    if (Event.reportObject.object == GENIE_OBJ_USER_LED) {
      if (Event.reportObject.index == 0) {
        bool UserLed0_val = genieGetEventData(&Event);
        UserLed0_val = !UserLed0_val;
        genieWriteObject(GENIE_OBJ_USER_LED, 0, UserLed0_val);
      }
    }
  }
}

/* UserApiConfig Handlers */
static bool uartAvailHandler(void) {
  
  return ROM_UARTCharsAvail(UART_BASE);
}

static uint8_t uartReadHandler(void){
  
  return (uint8_t)ROM_UARTCharGet(UART_BASE);
}

static void uartWriteHandler(uint32_t val) {
  
  ROM_UARTCharPut(UART_BASE,(uint8_t)val);
}

static uint32_t uartGetMillis(void) {
  
  return ROM_HibernateRTCGet() * MILLI_PER_SEC;
}
