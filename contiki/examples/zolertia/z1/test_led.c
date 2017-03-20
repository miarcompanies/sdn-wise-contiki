#include "contiki.h"
#include "dev/leds.h"
#include<stdio.h>
PROCESS(led_process, "LED process");
AUTOSTART_PROCESS(&led_process);
PROCESS_THREAD(led_process, ev, data){
	PROCESS_BEGIN();
	leds_on(LEDS_RED);
	PROCESS_END();
}
