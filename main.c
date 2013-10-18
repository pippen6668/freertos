#define USE_STDPERIPH_DRIVER
#include "stm32f10x.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

/* Filesystem includes */
#include "filesystem.h"
#include "fio.h"

extern const char _sromfs;

static void setup_hardware();

volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
volatile xQueueHandle serial_rx_queue = NULL;


/* IRQ handler to handle USART2 interruptss (both transmit and receive
 * interrupts). */
void USART2_IRQHandler()
{
	static signed portBASE_TYPE xHigherPriorityTaskWoken;
    char rx_msg;
	/* If this interrupt is for a transmit... */
	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		/* "give" the serial_tx_wait_sem semaphore to notfiy processes
		 * that the buffer has a spot free for the next byte.
		 */
		xSemaphoreGiveFromISR(serial_tx_wait_sem, &xHigherPriorityTaskWoken);

		/* Diables the transmit interrupt. */
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		/* If this interrupt is for a receive... */
	}

    else if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
       /* Receive the byte from the buffer. */
        rx_msg = USART_ReceiveData(USART2);

       /* Queue the received byte. */
        if(!xQueueSendToBackFromISR(serial_rx_queue, &rx_msg, &xHigherPriorityTaskWoken)) {
         /* If there was an error queueing the received byte,
           * freeze. */
         while(1);
        }
    }

	else {
		/* Only transmit and receive interrupts should be enabled.
		 * If this is another type of interrupt, freeze.
		 */
		while(1);
	}

	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}
}

void send_byte(char ch)
{
	/* Wait until the RS232 port can receive another byte (this semaphore
	 * is "given" by the RS232 port interrupt when the buffer has room for
	 * another byte.
	 */
	while (!xSemaphoreTake(serial_tx_wait_sem, portMAX_DELAY));

	/* Send the byte and enable the transmit interrupt (it is disabled by
	 * the interrupt).
	 */
	USART_SendData(USART2, ch);
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}
char receive_byte()
{
        //serial_ch_msg msg;
        char msg;
        /* Wait for a byte to be queued by the receive interrupts handler. */
        while (!xQueueReceive(serial_rx_queue, &msg, portMAX_DELAY));

        return msg;
}

size_t strlen ( const char * str )
{
    int count;
    for(count=0 ; str[count]!='\0';count++);
    return count;
 }

void and_shell()
{

        char str[100];
        char ch;
        char ch_type[2];
        char typein[]="and-freertos:";

        int curr_char;
        int done;

        /* Prepare the response message to be queued. */


        while (1) {

                fio_write(1,typein,strlen(typein)+1);
                curr_char = 0;
                done = 0;
                do {
                        /* Receive a byte from the RS232 port (this call will
                         * block). */

                        ch=receive_byte();
                        /* If the byte is an end-of-line type character, then
                         * finish the string and inidcate we are done.
                         */
                        if (curr_char >= 98 || (ch == '\r') || (ch == '\n')) {
                                str[curr_char] = '\n';
                                str[curr_char+1] = '\0';
                                done = -1;
                                /* Otherwise, add the character to the
                                 * response string. */
                        }

                        //backspace
                        else if(ch == 127)
                        {
                                if(curr_char != 0)
                            {
                                        curr_char--;
                                        fio_write(1,"\b \b",3);
                                }


                        }

                        else {
                                str[curr_char++] = ch;
                                fio_write(1,&ch,1);

                        }
                } while (!done);



                if( (curr_char++) >0)
                {
                       // Instruction(str);
                }
                //write(fdout, "\n\r", 3);

                /* Once we are done building the response string, queue the
                 * response to be sent to the RS232 port.
                 */

        }


}






int main()
{
	init_rs232();
	enable_rs232_interrupts();
	enable_rs232();

	fs_init();
	fio_init();

	register_romfs("romfs", &_sromfs);

	/* Create the queue used by the serial task.  Messages for write to
	 * the RS232. */
	vSemaphoreCreateBinary(serial_tx_wait_sem);
	serial_rx_queue = xQueueCreate(1, sizeof(char));
	xTaskCreate(and_shell,
              (signed portCHAR *) "and_shell",
              512 /* stack size */, NULL,
              tskIDLE_PRIORITY + 5, NULL);



	/* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}

void vApplicationTickHook()
{
}
