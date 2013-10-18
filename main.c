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

char* itoa(int num, char* string)
{
    int i=1;
        int num2=num;

        if(num2 < 10)
        {   string[0]=num%10+48;
            string[1]='\0';
            return string;
        }
    while(num2 > 9)
    {

        num2 /=10;
            i++;
        }

        num2=num;
        i=i-1;
        while(num2>0)
        {
          string[i]= (num2 %10)+48;
          num2/=10;
          i--;
        }

    string[i]='\0';
    return string;

}
int atoi(const char *string)
{
    int value = 0;
    for (; *string != '\0'; ++string)
        value = value*10 + *string - '0';
    return value;
}

int strcmp ( const char * str1, const char * str2)
{
   int i=0;
    while( (str1[i]!='\0' && str2[i]!='\0') )
    {
        if(str1[i]!=str2[i])
        {
            return (str1[i]>str2[i])?1:-1;
        }
        i++;
    }

    return 0;
 }
size_t strlen ( const char * str )
{
    int count;
    for(count=0 ; str[count]!='\0';count++);
    return count;
 }

void Instruction(char* str)
{



if( (strcmp("hello\n",str)) == 0 )
{
   fio_write(1, "\n\r'HELLO'",strlen("\n\r'HELLO'")+1 );
}

else if( (strcmp("echo ",str)) == 0 )
{
        int i=5;
        fio_write(1, "\n\r",2 );

        while(str[i] != '\n')
        {fio_write(1, str+i,1 );

         i++;
    }
}
}
/*
else if( (strcmp("ps\n",str,4)) == 0)
{
        int i;
        char string[6];
        write(fdout, "\n\r", 3);
        for(i = 0; i < task_count; i++)
    {
        itoa(tasks[i].pid, string);
        write(fdout, string, strlen(string)+1);
        write(fdout, "\t", 2);
                if(tasks[i].status == TASK_READY){write(fdout, "TASK READY\t", 12);}
                else if(tasks[i].status == TASK_WAIT_READ){write(fdout, "TASK_WAIT_READ\t", 16);}
                else if(tasks[i].status == TASK_WAIT_WRITE){write(fdout, "TASK_WAIT_WRITE\t", 17);}
                else if(tasks[i].status == TASK_WAIT_INTR){write(fdout, "TASK_WAIT_INTR\t", 16);}
                else if(tasks[i].status == TASK_WAIT_TIME){write(fdout, "TASK_WAIT_TIME\t", 16);}
                /*
                if(strncmp("TASK_READY",tasks[i].status,10) ){write(fdout, "TASK READY\t", 12);}
                else if(strncmp("TASK_WAIT_READ",tasks[i].status,14) ){write(fdout, "TASK_WAIT_READ\t", 16);}
                else if(strncmp("TASK_WAIT_WRITE",tasks[i].status,15)){write(fdout, "TASK_WAIT_WRITE\t", 17);}
                else if(strncmp("TASK_WAIT_INTR",tasks[i].status,14)){write(fdout, "TASK_WAIT_INTR\t", 16);}
                else if(strncmp("TASK_WAIT_TIME",tasks[i].status,14)){write(fdout, "TASK_WAIT_TIME\t", 16);}

        itoa(tasks[i].priority, string);
        write(fdout, string, strlen(str)+1);
        write(fdout, "\n\r",3);
        }

}
else
 {
   write(fdout,"\n\rinput error,type help to get more information",48);
 }

}
*/


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
                       Instruction(str);
                }
                //write(fdout, "\n\r", 3);
                fio_write(1,"\n\r",2);
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
