### TODO

- [x] Port to c
- [x] Add TI example in IAR
- [ ] Add Nordic example
- [ ] Add Makefile
- [ ] Add STM32 example
- [ ] Add general documentation
- [ ] Add back overloaded writeStr methods removed by port

<br>

This branch has been created in order to add additional board support to [Visi-Genie-Arduino-Library](https://github.com/4dsystems/ViSi-Genie-Arduino-Library) by 
4DSystems. The goal is to port their C++ library to C, adding a generic interface for peripheral based functions. For 
example, sending a character over UART is handled different based on the vendor:


**Arduino:**    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _Serial.Write_

**TI:**        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  _ROM_UARTCharPut()_
 
**Nordic:**    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _app_uart_put()_

**STM**       &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _HAL_UART_Transmit_

The application specific calls can be configured outside the library by the use of the UserApiConfig struct. For 
example, the TM4C129 Launchpad by TI:


````
static bool uartAvailHandler(void) {
  
  return ROM_UARTCharsAvail(UART_BASE);
}

static uint8_t uartReadHandler(void){
  
  return (uint8_t)ROM_UARTCharGet(UART_BASE);
}

static void uartWriteHandler(uint32_t val) {
  
  ROM_UARTCharPut(UART_BASE,(uint8_t)val);
}

static uint32_t getMillis(void) {
  
  return ROM_HibernateRTCGet() * MILLI_PER_SEC;
}

int main(void) {
    ...
    ...
  static UserApiConfig userConfig = {
    .available = uartAvailHandler,
    .read =  uartReadHandler,
    .write = uartWriteHandler,
    .millis = getMillis
  };
  
  initGenieWithConfig(&userConfig);
    ...
    ...
````
<br>
For more information on 4DSystems Visi-Genie-Arduino-Library [click here](https://github.com/4dsystems/ViSi-Genie-Arduino-Library)