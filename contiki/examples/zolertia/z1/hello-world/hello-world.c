#include "contiki.h"
#include<stdio.h>
PROCESS(hello_world, "Hello world process");
AUTOSTART_PROCESSES(&hello_world);
PROCESS_THREAD(hello_world, ev, data){
	PROCESS_BEGIN();
	printf("Hello, world\n");
	PROCESS_END();
}
