/*
 * Copyright (C) 2015 SDN-WISE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *         SDN-WISE for Contiki.
 * \author
 *         Sebastiano Milardo <s.milardo@hotmail.it>
 */

/**
 * \addtogroup sdn-wise
 * @{
 */

#include "contiki.h"
#include "net/rime/rime.h"
#include "net/linkaddr.h"  
#include "dev/watchdog.h"
//#include "dev/uart1.h"
#include "dev/uart0.h"
#include "dev/leds.h"  
#include "lib/list.h"
#if CFS_ENABLED
#include "cfs/cfs.h"
#endif
#if ELF_ENABLED
#include "loader/elfloader.h"
#endif
#include "sdn-wise.h"
#include "flowtable.h"
#include "packet-buffer.h"
#include "packet-handler.h"
#include "packet-creator.h"
#include "neighbor-table.h"
#include "node-conf.h"

#define UART_BUFFER_SIZE      MAX_PACKET_LENGTH

#define UNICAST_CONNECTION_NUMBER     29
#define BROADCAST_CONNECTION_NUMBER       30

#define TIMER_EVENT       50
//Million Dynamic Topo
#define UPDATE_TOPO_EVENT 	  60
#define RF_U_SEND_EVENT     51
#define RF_B_SEND_EVENT     52
#define RF_U_RECEIVE_EVENT    53
#define RF_B_RECEIVE_EVENT    54
#define UART_RECEIVE_EVENT      55
#define RF_SEND_BEACON_EVENT  56
#define RF_SEND_REPORT_EVENT  57
#define NEW_PACKET_EVENT  58
#define ACTIVATE_EVENT    59

#define DEBUG 1
#if DEBUG && (!SINK || DEBUG_SINK)
#include <stdio.h>
#include <inttypes.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
//show define
#define STR(x) #x
#define SHOW_DEFINE(x) printf("%s=%s\n", #x, STR(x))
/*----------------------------------------------------------------------------*/
  PROCESS(main_proc, "Main Process");
  PROCESS(rf_u_send_proc, "RF Unicast Send Process");
  PROCESS(rf_b_send_proc, "RF Broadcast Send Process");
  PROCESS(packet_handler_proc, "Packet Handler Process");
  PROCESS(timer_proc, "Timer Process");
  //PROCESS(update_topo_proc, "Update Topology Process");
  PROCESS(beacon_timer_proc, "Beacon Timer Process");
  PROCESS(report_timer_proc, "Report Timer Process");
  AUTOSTART_PROCESSES(
    &main_proc,
    &rf_u_send_proc,
    &rf_b_send_proc,
    &timer_proc,
    //&update_topo_proc,
    &beacon_timer_proc,
    &report_timer_proc,
    &packet_handler_proc
    );
/*----------------------------------------------------------------------------*/
  static uint8_t uart_buffer[UART_BUFFER_SIZE];
  static uint8_t uart_buffer_index = 0;
  static uint8_t uart_buffer_expected = 0; 
  static uint8_t tmp_uart_buffer[5];
  static uint8_t copy_to_tmp = 0;
  static uint8_t tmp_index = 0;
  uint8_t data_packet_counter = 0;
/*----------------------------------------------------------------------------*/
  void
  rf_unicast_send(packet_t* p)
  {
    process_post(&rf_u_send_proc, RF_U_SEND_EVENT, (process_data_t)p);
  }
/*----------------------------------------------------------------------------*/
  void
  rf_broadcast_send(packet_t* p)
  {
    process_post(&rf_b_send_proc, RF_B_SEND_EVENT, (process_data_t)p);
  }
/*----------------------------------------------------------------------------*/
  static void
  unicast_rx_callback(struct unicast_conn *c, const linkaddr_t *from)
  {
    packet_t* p = get_packet_from_array((uint8_t *)packetbuf_dataptr());
    if (p != NULL){
      // TODO the exact rssi value depends on the radio (search for a formula)
      p->info.rssi = (uint8_t) (- packetbuf_attr(PACKETBUF_ATTR_RSSI));
      process_post(&main_proc, RF_U_RECEIVE_EVENT, (process_data_t)p);
    }
  }

/*----------------------------------------------------------------------------*/
  static void
  broadcast_rx_callback(struct broadcast_conn *c, const linkaddr_t *from)
  {
    packet_t* p = get_packet_from_array((uint8_t *)packetbuf_dataptr());
    if (p != NULL){
      // TODO the exact rssi value depends on the radio (search for a formula)
      // http://sourceforge.net/p/contiki/mailman/message/31805752/
      p->info.rssi = (uint8_t) (- packetbuf_attr(PACKETBUF_ATTR_RSSI));
      process_post(&main_proc, RF_B_RECEIVE_EVENT, (process_data_t)p);
    }
  }
/*----------------------------------------------------------------------------*/
  int
  uart_rx_callback(unsigned char c)
  {
    // TODO works with cooja, will not work with real nodes, cause -> syn
    if(uart_buffer_index == UART_BUFFER_SIZE)
    	uart_buffer_index = 0;
    uart_buffer[uart_buffer_index] = c;
    if(uart_buffer_index<5)
    	tmp_uart_buffer[uart_buffer_index] = uart_buffer[uart_buffer_index]; 
    //if (uart_buffer_index == LEN_INDEX){
      //Million: reduced size of expected for fast printing
      //uart_buffer_expected = c;
      //uart_buffer_expected = 6;
    //}
    if(copy_to_tmp == 1 && tmp_index<5){
        tmp_uart_buffer[tmp_index] = uart_buffer[uart_buffer_index];
        tmp_index ++;
    }
    if(c == 10){ //newline \n
	copy_to_tmp = 1;
        tmp_index = 0;
    }
    uart_buffer_index++;
    if((tmp_index == 5 || uart_buffer_index == 5) && ((tmp_uart_buffer[0] == 100) || (tmp_uart_buffer[2] == 117 || tmp_uart_buffer[2] == 98 || tmp_uart_buffer[2] == 100) || (tmp_uart_buffer[2]== 114 && tmp_uart_buffer[3] == 102) || (tmp_uart_buffer[2]== 115 && tmp_uart_buffer[3] == 102) || (tmp_uart_buffer[2] == 116 && tmp_uart_buffer[4] == 114) )){ 
	copy_to_tmp = 0;
        tmp_index = 0;
    //if (uart_buffer_index == uart_buffer_expected){
       // uart_buffer_index = 0;
       // uart_buffer_expected = 0;
      //Million: trying to create config packet
      //packet_t* p = get_packet_from_array(uart_buffer);
      packet_t* p = create_packet_empty();
      p->header.net = conf.my_net;
      //set_broadcast_address(&(p->header.dst));
      if(tmp_uart_buffer[0] == 49 && tmp_uart_buffer[1] == 49){ //'1'
      	p->header.dst.u8[0] = 2;
      	p->header.dst.u8[1] = 0;
      }
      else if(tmp_uart_buffer[0] == 50 && tmp_uart_buffer[1] == 50){//'2'
	p->header.dst.u8[0] = 3;
        p->header.dst.u8[1] = 0;
      }
      else if(tmp_uart_buffer[0] == 51 && tmp_uart_buffer[1] == 51){ //'3'
        p->header.dst.u8[0] = 4;
        p->header.dst.u8[1] = 0;
      }
      else if(tmp_uart_buffer[0] == 52 && tmp_uart_buffer[1] == 52){//'4'
        p->header.dst.u8[0] = 5;
        p->header.dst.u8[1] = 0;
      }
      else if(tmp_uart_buffer[0] == 53 && tmp_uart_buffer[1] == 53){//'5'
        p->header.dst.u8[0] = 6;
        p->header.dst.u8[1] = 0;
      }
      else if(tmp_uart_buffer[0] == 54 && tmp_uart_buffer[1] == 54){//'6'
        p->header.dst.u8[0] = 7;
        p->header.dst.u8[1] = 0;
      }
      else if(tmp_uart_buffer[0] == 55 && tmp_uart_buffer[1] == 55){//'7'
        p->header.dst.u8[0] = 8;
        p->header.dst.u8[1] = 0;
      }
      else if(tmp_uart_buffer[0] == 56 && tmp_uart_buffer[1] == 56){//'8'
        p->header.dst.u8[0] = 9;
        p->header.dst.u8[1] = 0;
      }
      else if(tmp_uart_buffer[0] == 57 && tmp_uart_buffer[1] == 57){//'9'
        p->header.dst.u8[0] = 10;
        p->header.dst.u8[1] = 0;
      }
      /*else if(tmp_uart_buffer[0] == 49 && tmp_uart_buffer[1] == 48){//'10'
        p->header.dst.u8[0] = 11;
        p->header.dst.u8[1] = 0;
      }*/

      else{
	p->header.dst.u8[0] = 1;
        p->header.dst.u8[1] = 0;
      }
      p->header.src = conf.my_address;
      //p->header.typ = CONFIG;
      //set to config later
      if(tmp_uart_buffer[0] == 100){ //dd for data d in ascii is 100
	p->header.typ = DATA;
	if(tmp_uart_buffer[3] == 48 && tmp_uart_buffer[4] == 48){ //'00'
	        p->header.dst.u8[0] = 1;
		p->header.dst.u8[1] = 0;
	}
        else if(tmp_uart_buffer[3] == 49 && tmp_uart_buffer[4] == 49){ //'11'
        	p->header.dst.u8[0] = 2;
	        p->header.dst.u8[1] = 0;
	}
        else if(tmp_uart_buffer[3] == 50 && tmp_uart_buffer[4] == 50){//'2'
          p->header.dst.u8[0] = 3;
          p->header.dst.u8[1] = 0;
        }
        else if(tmp_uart_buffer[3] == 51 && tmp_uart_buffer[4] == 51){ //'3'
          p->header.dst.u8[0] = 4;
          p->header.dst.u8[1] = 0;
        }
        else if(tmp_uart_buffer[3] == 52 && tmp_uart_buffer[4] == 52){//'4'
          p->header.dst.u8[0] = 5;
          p->header.dst.u8[1] = 0;
        }
        else if(tmp_uart_buffer[3] == 53 && tmp_uart_buffer[4] == 53){//'5'
          p->header.dst.u8[0] = 6;
          p->header.dst.u8[1] = 0;
        }
	else if(tmp_uart_buffer[3] == 54 && tmp_uart_buffer[4] == 54){//'6'
          p->header.dst.u8[0] = 7;
          p->header.dst.u8[1] = 0;
        }
	else if(tmp_uart_buffer[3] == 55 && tmp_uart_buffer[4] == 55){//'7'
          p->header.dst.u8[0] = 8;
          p->header.dst.u8[1] = 0;
        }
	else if(tmp_uart_buffer[3] == 56 && tmp_uart_buffer[4] == 56){//'8'
          p->header.dst.u8[0] = 9;
          p->header.dst.u8[1] = 0;
        }
	else if(tmp_uart_buffer[3] == 57 && tmp_uart_buffer[4] == 57){//'9'
          p->header.dst.u8[0] = 10;
          p->header.dst.u8[1] = 0;
        }
	/*else if(tmp_uart_buffer[3] == 49 && tmp_uart_buffer[4] == 48){//'10'
          p->header.dst.u8[0] = 11;
          p->header.dst.u8[1] = 0;
        }*/
        else{
          p->header.dst.u8[0] = 1;
          p->header.dst.u8[1] = 0;
       }
       if(tmp_uart_buffer[1] == 48 && tmp_uart_buffer[2] == 48){//'00'
          p->header.src.u8[0] = 1;
          p->header.src.u8[1] = 0;
       }
       else if(tmp_uart_buffer[1] == 49 && tmp_uart_buffer[2] == 49){//'11'
          p->header.src.u8[0] = 2;
          p->header.src.u8[1] = 0;
       } 
       else if(tmp_uart_buffer[1] == 50 && tmp_uart_buffer[2] == 50){//'22'
          p->header.src.u8[0] = 3;
          p->header.src.u8[1] = 0;
        }
	else if(tmp_uart_buffer[1] == 51 && tmp_uart_buffer[2] == 51){//'33'
          p->header.src.u8[0] = 4;
          p->header.src.u8[1] = 0;
        }
	else if(tmp_uart_buffer[1] == 52 && tmp_uart_buffer[2] == 52){//'44'
          p->header.src.u8[0] = 5;
          p->header.src.u8[1] = 0;
        }
	else if(tmp_uart_buffer[1] == 53 && tmp_uart_buffer[2] == 53){//'55'
          p->header.src.u8[0] = 6;
          p->header.src.u8[1] = 0;
        }
	else if(tmp_uart_buffer[1] == 54 && tmp_uart_buffer[2] == 54){//'66'
          p->header.src.u8[0] = 7;
          p->header.src.u8[1] = 0;
        }
	else if(tmp_uart_buffer[1] == 55 && tmp_uart_buffer[2] == 56){//'77'
          p->header.src.u8[0] = 8;
          p->header.src.u8[1] = 0;
        }
	else if(tmp_uart_buffer[1] == 56 && tmp_uart_buffer[2] == 57){//'88'
          p->header.src.u8[0] = 9;
          p->header.src.u8[1] = 0;
        }
	else if(tmp_uart_buffer[1] == 57 && tmp_uart_buffer[2] == 57){//'99'
          p->header.src.u8[0] = 10;
          p->header.src.u8[1] = 0;
        }
	/*else if(tmp_uart_buffer[1] == 49 && tmp_uart_buffer[2] == 48){//'10'
          p->header.src.u8[0] = 11;
          p->header.src.u8[1] = 0;
        }*/
      }
      else{ 
	p->header.typ = CONFIG;
      }
      //p->header.nxh = conf.nxh_vs_sink;
      //p->header.nxh = p->header.dst;
      //Million: for demo purpose we will send all config and data packets to Node 1
/*#if SINK
      p->header.nxh.u8[0] = 2;
      p->header.nxh.u8[1] = 0; 
      //p->header.nxh = p->header.dst;
#else
      p->header.nxh = conf.nxh_vs_sink;
#endif*/
      p->header.nxh = conf.my_address;
      //address_t tmp  = nearest_neighbor();
      //p->header.nxh = tmp;
      //neighbor_t* tmp_neighbor = neighbor_table_contains(&p->header.dst);
      //if(tmp_neighbor != NULL)     
      //p->header.nxh = p->header.dst;
      set_payload_at(p, 0, tmp_uart_buffer[0]);
      set_payload_at(p, 1, tmp_uart_buffer[1]);
      set_payload_at(p, 2, tmp_uart_buffer[2]);
      set_payload_at(p, 3, tmp_uart_buffer[3]);
      set_payload_at(p, 4, tmp_uart_buffer[4]);
      //rf_broadcast_send(p);
      if (p != NULL){
        p->info.rssi = 255;
        if(p->header.typ == DATA){
		set_payload_at(p, 5, data_packet_counter);
		print_report_data(tmp_uart_buffer[1], tmp_uart_buffer[2], tmp_uart_buffer[3], tmp_uart_buffer[4]);
		PRINTF("Send Data Packet %d\n", data_packet_counter);
		data_packet_counter++;
        }
        else{
		print_report_config(tmp_uart_buffer[0], tmp_uart_buffer[1], tmp_uart_buffer[3], tmp_uart_buffer[4]);
	}
        process_post(&main_proc, UART_RECEIVE_EVENT, (process_data_t)p);  
        //rf_unicast_send(p);
      }
    }
    return 0;
  }
/*----------------------------------------------------------------------------*/
  static const struct unicast_callbacks unicast_callbacks = {
	unicast_rx_callback 
  };
  static struct unicast_conn uc;
  static const struct broadcast_callbacks broadcast_callbacks = { 
    broadcast_rx_callback 
  };
  static struct broadcast_conn bc;
/*----------------------------------------------------------------------------*/
  PROCESS_THREAD(main_proc, ev, data) {
    PROCESS_BEGIN();
	
    //uart1_init(BAUD2UBR(115200));       /* set the baud rate as necessary */
    //uart1_set_input(uart_rx_callback);  /* set the callback function */    

    uart0_init(BAUD2UBR(115200));       /* set the baud rate as necessary */
    uart0_set_input(uart_rx_callback);  /* set the callback function */
    
    node_conf_init();
    flowtable_init();
    packet_buffer_init();
    neighbor_table_init();
    address_list_init();
    leds_init();
#if SINK
    print_packet_uart(create_reg_proxy());
#endif    

    while(1) {
      PROCESS_WAIT_EVENT();
      switch(ev) {
        case TIMER_EVENT:
     // it was commented
       //test_handle_open_path();
       //test_flowtable();
       //test_neighbor_table();
       //test_packet_buffer();
       //test_address_list();
       //print_node_conf();
	//Million Added to display neighbor table and send data
	PRINTF("Neighbor Table\n");
        print_neighbor_table();
	reset_isalive_neighbor();
	PRINTF("Neighbor Table\n");
        print_neighbor_table();
        break;

        case UPDATE_TOPO_EVENT:
 	PRINTF("Updating Topology Neighbors\n");
	PRINTF("Neighbor Table\n");
        print_neighbor_table();
	update_topo_neighbor();
	PRINTF("Neighbor Table\n");
        print_neighbor_table();
        break;
        
        case UART_RECEIVE_EVENT:
        leds_toggle(LEDS_GREEN);
        process_post(&packet_handler_proc, NEW_PACKET_EVENT, (process_data_t)data);
	//packet_t* p = (packet_t*)data;
        //rf_unicast_send(p);
        break;

        case RF_B_RECEIVE_EVENT:
        leds_toggle(LEDS_YELLOW);
        if (!conf.is_active){
          conf.is_active = 1;
          process_post(&beacon_timer_proc, ACTIVATE_EVENT, (process_data_t)NULL);
          process_post(&report_timer_proc, ACTIVATE_EVENT, (process_data_t)NULL);
        }
 	//Million I suggest to have break
        //break;
        case RF_U_RECEIVE_EVENT:
        process_post(&packet_handler_proc, NEW_PACKET_EVENT, (process_data_t)data);
        break;

        case RF_SEND_BEACON_EVENT:
        leds_toggle(LEDS_RED);
	PRINTF("Beacon Send ");
        rf_broadcast_send(create_beacon());
        break;

        case RF_SEND_REPORT_EVENT:
        leds_toggle(LEDS_RED);
	PRINTF("Send Report\n");
#if !SINK
        rf_unicast_send(create_report());
#else
	//Million SINK dones't sends report also to controller
	PRINTF("SINK Sending Report - To Controller(Method will be developed)\n");
 	send_report_to_controller(create_report());
	//send_request_to_controller(create_report());
#endif
	//rf_broadcast_send(create_report());
        break;
      } 
    }
    PROCESS_END();
  }
/*----------------------------------------------------------------------------*/
  PROCESS_THREAD(rf_u_send_proc, ev, data) {
    static linkaddr_t addr;
    PROCESS_EXITHANDLER(unicast_close(&uc);)
    PROCESS_BEGIN();
    unicast_open(&uc, UNICAST_CONNECTION_NUMBER, &unicast_callbacks);
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(ev == RF_U_SEND_EVENT);
      packet_t* p = (packet_t*)data;

      if (p != NULL){
        p->header.ttl--;
        if (!is_my_address(&(p->header.dst))){
          int i = 0;

          int sent_size = 0;

          if (LINKADDR_SIZE < ADDRESS_LENGTH){
            sent_size = LINKADDR_SIZE;
          } else {
            sent_size = ADDRESS_LENGTH;
          }

          for ( i=0; i<sent_size; ++i){
            addr.u8[i] = p->header.nxh.u8[(sent_size-1)-i];
          }
 	  addr.u8[0] = p->header.nxh.u8[0];
          addr.u8[1] = p->header.nxh.u8[1];
          //Million added next two lines for debugging
 	  PRINTF("[TXU]: ");
	  print_packet(p);
	  uint8_t* a = (uint8_t*)p;
          packetbuf_copyfrom(a,p->header.len);
          unicast_send(&uc, &addr);
        } 
#if SINK
        else {
		//Million A.
        	if(p->header.typ == REPORT)
                	PRINTF("SINK Sending Report Packet - To Controller(Method will be developed)\n");
		//print_packet_uart(p);
		send_report_to_controller(p);
	}
#endif
        packet_deallocate(p);
      }
    }
    PROCESS_END();
  }
/*----------------------------------------------------------------------------*/
  PROCESS_THREAD(rf_b_send_proc, ev, data) {
    PROCESS_EXITHANDLER(broadcast_close(&bc);)
    PROCESS_BEGIN();
    broadcast_open(&bc, BROADCAST_CONNECTION_NUMBER, &broadcast_callbacks);
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(ev == RF_B_SEND_EVENT);
      packet_t* p = (packet_t*)data;

      if (p != NULL){
        p->header.ttl--;
        PRINTF("[TXB]: ");
        print_packet(p);
        PRINTF("\n");
        
        uint8_t* a = (uint8_t*)p;
        packetbuf_copyfrom(a,p->header.len);  
        broadcast_send(&bc);
        packet_deallocate(p);
      }
    }
    PROCESS_END();
  }
/*----------------------------------------------------------------------------*/
  PROCESS_THREAD(timer_proc, ev, data) {
    static struct etimer et;
    static struct etimer et_update;
    PROCESS_BEGIN();

    while(1) {
      //Million slow the timer from 3 to 15
      //etimer_set(&et, 3 * CLOCK_SECOND);
      etimer_set(&et, 15 * CLOCK_SECOND);
      //etimer_set(&et, 15 * RTIMER_ARCH_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      //Million reset timer to display neighbor table every 15 seconds
      etimer_reset(&et);
      process_post(&main_proc, TIMER_EVENT, (process_data_t)NULL);
      //Million Update Topo	
      etimer_set(&et_update, 10 * CLOCK_SECOND);
      //etimer_set(&et, 25 * RTIMER_ARCH_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_update));
       //Million reset timer to display neighbor table every 15 seconds
      etimer_reset(&et_update);
      process_post(&main_proc, UPDATE_TOPO_EVENT, (process_data_t)NULL);
    }
    PROCESS_END();
  }

  /*PROCESS_THREAD(update_topo_proc, ev, data) {
    static struct etimer et;
    PROCESS_BEGIN();

    while(1) {
      //Million slow the timer from 3 to 15
      //etimer_set(&et, 3 * CLOCK_SECOND);
      etimer_set(&et, 25 * CLOCK_SECOND);
      //etimer_set(&et, 25 * RTIMER_ARCH_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      //Million reset timer to display neighbor table every 15 seconds
      etimer_reset(&et);
      process_post(&main_proc, UPDATE_TOPO_EVENT, (process_data_t)NULL);
    }
    PROCESS_END();
  }*/

/*----------------------------------------------------------------------------*/
  PROCESS_THREAD(beacon_timer_proc, ev, data) {
    static struct etimer et;
    
    PROCESS_BEGIN();
    while(1){
#if !SINK
      if (!conf.is_active){
        PROCESS_WAIT_EVENT_UNTIL(ev == ACTIVATE_EVENT);
      }
#endif
      etimer_set(&et, conf.beacon_period * CLOCK_SECOND);
      //etimer_set(&et, conf.beacon_period * RTIMER_ARCH_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      process_post(&main_proc, RF_SEND_BEACON_EVENT, (process_data_t)NULL);
    }
    PROCESS_END();
  }
/*----------------------------------------------------------------------------*/
  PROCESS_THREAD(report_timer_proc, ev, data) {
    static struct etimer et;
    
    PROCESS_BEGIN();
    while(1){
#if !SINK
      if (!conf.is_active){
        PROCESS_WAIT_EVENT_UNTIL(ev == ACTIVATE_EVENT);
      }
#endif
      etimer_set(&et, conf.report_period * CLOCK_SECOND);
      //etimer_set(&et, conf.report_period * RTIMER_ARCH_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      process_post(&main_proc, RF_SEND_REPORT_EVENT, (process_data_t)NULL);
    }
    PROCESS_END();
  }
/*----------------------------------------------------------------------------*/
  PROCESS_THREAD(packet_handler_proc, ev, data) {
    PROCESS_BEGIN();
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(ev == NEW_PACKET_EVENT);
      packet_t* p = (packet_t*)data;
      PRINTF("[RX]: ");
      print_packet(p);
      PRINTF("\n");
      handle_packet(p);
    }
    PROCESS_END();
  }
/*----------------------------------------------------------------------------*/
/** @} */
