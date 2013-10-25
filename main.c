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

#define CIRCBUFSIZE 5000
unsigned int write_pointer, read_pointer;

static unsigned int lfsr = 0xACE1;

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
       
        char msg;
        /* Wait for a byte to be queued by the receive interrupts handler. */
        while (!xQueueReceive(serial_rx_queue, &msg, portMAX_DELAY));

        return msg;
}
struct slot {
    void *pointer;
    unsigned int size;
    unsigned int lfsr;
};
static struct slot slots[CIRCBUFSIZE];


static unsigned int circbuf_size(void)
{
    return (write_pointer + CIRCBUFSIZE - read_pointer) % CIRCBUFSIZE;
}

static void write_cb(struct slot foo)
{
    if (circbuf_size() == CIRCBUFSIZE - 1) {
        //fprintf(stderr, "circular buffer overflow\n");
		fio_write(1,"circular buffer overflow\n",25);
       return;
    }
    slots[write_pointer++] = foo;
    write_pointer %= CIRCBUFSIZE;
}

static struct slot read_cb(void)
{
    struct slot foo;
    if (write_pointer == read_pointer) {
        // circular buffer is empty
        return (struct slot){ .pointer=NULL, .size=0, .lfsr=0 };
    }
    foo = slots[read_pointer++];
    read_pointer %= CIRCBUFSIZE;
    return foo;
}


// Get a pseudorandom number generator from Wikipedia
static int prng(void)
{
    static unsigned int bit;
    /* taps: 16 14 13 11; characteristic polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
    bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
    lfsr =  (lfsr >> 1) | (bit << 15);
    return lfsr & 0xffff;
}
void mmtest()
{   
    int i, size;
    char *p;

    while (1) {
        size = prng() & 0x7FF;
        fio_printf("try to allocate %d bytes", size);
        p = (char *) pvPortMalloc(size);
        //fio_printf("malloc returned %p\n", p);
        if (p == NULL) {
            // can't do new allocations until we free some older ones
            while (circbuf_size() > 0) {
                // confirm that data didn't get trampled before freeing
                struct slot foo = read_cb();
                p = foo.pointer;
                lfsr = foo.lfsr;  // reset the PRNG to its earlier state
                size = foo.size;
                fio_printf("free a block, size %d", size);
                for (i = 0; i < size; i++) {
                    unsigned char u = p[i];
                    unsigned char v = (unsigned char) prng();
                    if (u != v) {
                        
                        fio_printf("OUCH: u=%d, v=%d", u, v);
						return ;
                    }
                }
                vPortFree(p);
                if ((prng() & 1) == 0) break;
            }
        } else {
            fio_printf("allocate a block, size %d", size);
            write_cb((struct slot){.pointer=p, .size=size, .lfsr=lfsr});
            for (i = 0; i < size; i++) {
                p[i] = (unsigned char) prng();
            }
        }
    }
	

}






void shell(char* str)
{



if( (strcmp("hello\n",str)) == 0 )
{
   fio_write(1, "\n\r'HELLO'",9 );
}

else if( (strcmp("echo ",str)) == 0 )
{
        int i=5;
        fio_write(1, "\n\r",2 );

        while(str[i] != '\n')
        {
		 
		 fio_write(1, str+i,1 );
		 i++;
		 
    }
}


else if( (strcmp("ps\n",str)) == 0)
{
        
        fio_write(1, "\n\r",2 );
		char str[100];
		vTaskList(str);
		fio_write(1,str,strlen(str)); 
   

}
else if ((strcmp("cat",str)) == 0)
{
    int i=4;
	
    char cat_bf[128];
    cat_bf[0]='\0';

	size_t count;

	char str2[10]={};
    int a=0;
	
	fio_write(1, "\r\n", 2);
	while(str[i] != '\0')
	{ 
	  
	  if(str[i] !='\n' && str[i] !=' ')
	  { 
	    str2[a]=str[i];
		a++;	
	  }
	  
	  else{ 
	    
	    str2[a]='\0';
		char path[20]="/romfs/";
	     strcat(path,str2);
	     int fd = fs_open(path, 0, O_RDONLY);
		 if(fd>0){
	     do
           {
            count = fio_read(fd,cat_bf, sizeof( cat_bf));
            fio_write(1, cat_bf, count);
           }while (count);
		   }
		   fio_write(1, "\r", 1);
		a=0;
	  } 
	  i++;
	}
}
else if((strcmp("mmtest",str)) == 0)
{   mmtest();

}



}




void type_in()
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
                       shell(str);
                }
                
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
	xTaskCreate(type_in,
              (signed portCHAR *) "type_in",
              512 /* stack size */, NULL,
              tskIDLE_PRIORITY + 5, NULL);



	/* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}

void vApplicationTickHook()
{
}
