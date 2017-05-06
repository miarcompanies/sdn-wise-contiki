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
 *         SDN-WISE Packet Handler.
 * \author
 *         Sebastiano Milardo <s.milardo@hotmail.it>
 */

/**
 * \addtogroup sdn-wise
 * @{
 */

#include <string.h>
#include <stdio.h>
#include "contiki.h"
#include "dev/watchdog.h"
#include "packet-handler.h"
#include "address.h"
#include "packet-buffer.h"
#include "packet-creator.h"
#include "neighbor-table.h"
#include "flowtable.h"
#include "node-conf.h"
#include "sdn-wise.h"
#include "net/rime/rime.h"
typedef enum conf_id{
  RESET,
  MY_NET,
  MY_ADDRESS,
  PACKET_TTL,
  RSSI_MIN,
  BEACON_PERIOD,
  REPORT_PERIOD,
  RULE_TTL,
  ADD_ALIAS,
  REM_ALIAS,  
  GET_ALIAS,
  ADD_RULE,
  REM_RULE,
  GET_RULE,
  ADD_FUNCTION,
  REM_FUNCTION,
  GET_FUNCTION
} conf_id_t;

const uint8_t conf_size[RULE_TTL+1] = 
{
  0,
  sizeof(conf.my_net),              
  sizeof(conf.my_address),          
  sizeof(conf.packet_ttl),
  sizeof(conf.rssi_min),
  sizeof(conf.beacon_period),       
  sizeof(conf.report_period),       
  sizeof(conf.rule_ttl)
};

const void* conf_ptr[RULE_TTL+1] = 
{
  NULL,
  &conf.my_net,              
  &conf.my_address,          
  &conf.packet_ttl,
  &conf.rssi_min,
  &conf.beacon_period,       
  &conf.report_period,       
  &conf.rule_ttl,              
};

#define CNF_READ 0
#define CNF_WRITE 1

#define DEBUG 1
#if DEBUG && (!SINK || DEBUG_SINK)
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*----------------------------------------------------------------------------*/
  static void handle_beacon(packet_t*);
  static void handle_data(packet_t*);
  static void handle_report(packet_t*);
  static void handle_response(packet_t*);
  static void handle_open_path(packet_t*);
  static void handle_config(packet_t*);
  static void handle_request(packet_t*);
/*----------------------------------------------------------------------------*/
  void 
  handle_packet(packet_t* p)
  {
    //Million A: check if report packet is arriving
      if (p->info.rssi >= conf.rssi_min && p->header.net == conf.my_net){
      if (p->header.typ == BEACON){
        PRINTF("[PHD]: Beacon %d from %d.%d\n", get_payload_at(p,2), p->header.src.u8[0], p->header.src.u8[1]);
        handle_beacon(p);
      } else {
	if(is_my_address(&(p->header.nxh))){
	  switch (p->header.typ){
            case DATA:
            PRINTF("[PHD]: Data\n");
            handle_data(p);
            break;

            case RESPONSE:
            PRINTF("[PHD]: Response\n");
            handle_response(p);
            break;

            case OPEN_PATH:
            PRINTF("[PHD]: Open Path\n");
            handle_open_path(p);
            break;

            case CONFIG:
            PRINTF("[PHD]: Config\n");
            handle_config(p);
            break;

            case REQUEST:
	    PRINTF("[PHD]: Request\n");
	    handle_request(p);
            break;

            default:
            PRINTF("[PHD]: Report\n");
            handle_report(p);
            break;
          }
	}
	else
		match_packet(p);
      }
    } else {
      packet_deallocate(p);
    }
  }
/*----------------------------------------------------------------------------*/
  
  void
  handle_beacon(packet_t* p)
  {
   //Million remove neighbor if rssi value is minimum
   add_neighbor(&(p->header.src),p->info.rssi, 1);
#if !SINK  
  // TODO what if the network changes?
    
    PRINTF("Before: Hops = %d NXH: %d.%d\n", conf.hops_from_sink, conf.nxh_vs_sink.u8[0], conf.nxh_vs_sink.u8[1]);
    uint8_t new_hops = get_payload_at(p, BEACON_HOPS_INDEX);
    PRINTF("New Hops: %d New RSSI: %d Existing RSSI: %d\n", new_hops, p->info.rssi, conf.rssi_from_sink);
    if (new_hops < conf.hops_from_sink-1 || (new_hops == conf.hops_from_sink-1 &&
    	//Million Aregawi: modified lower rssi value is strong
//      p->info.rssi > conf.rssi_from_sink)
	p->info.rssi < conf.rssi_from_sink))
    {
      conf.nxh_vs_sink = p->header.src;
      conf.hops_from_sink = new_hops+1;
      conf.rssi_from_sink = p->info.rssi;  
      conf.sink_address = p->header.nxh;
      PRINTF("After: Hops = %d NXH: %d.%d\n", conf.hops_from_sink, conf.nxh_vs_sink.u8[0], conf.nxh_vs_sink.u8[1]);
    }
#endif
    packet_deallocate(p);
  }
/*----------------------------------------------------------------------------*/
  void
  handle_data(packet_t* p)
  {
    if (is_my_address(&(p->header.dst)))
    {     
      PRINTF("Data Packet %d from %d.%d Arrived\n", get_payload_at(p,5), p->header.src.u8[0], p->header.src.u8[1]);
      PRINTF("[PHD]: Consuming Packet\n");
      packet_deallocate(p);
    } else {
      match_packet(p);
    }
  }
/*----------------------------------------------------------------------------*/
  void
  handle_report(packet_t* p)
  {
#if SINK
      //Million Added
      PRINTF("I got a Report, Sending To Controller From Sink\n");
      //print_packet_uart(p);
      send_report_to_controller(p);
#else 
    
    p->header.nxh = conf.nxh_vs_sink;
    rf_unicast_send(p);
#endif  
  }

  void
  handle_request(packet_t* p)
  {
#if SINK
      //Million Added
      PRINTF("I got a Request, Sending Request to Controller\n");
      /*packet_t* r = create_packet_empty();
      if (r != NULL){
      	r->header.net = conf.my_net;
	r->header.dst = p->header.src;
        //r->header.dst.u8[0] = get_payload_at(p,5);
        //r->header.dst.u8[1] = get_payload_at(p,6);
	r->header.typ = CONFIG;
        r->header.nxh = r->header.dst;
        //r->header.nxh = conf.nxh_vs_sink;
        r->header.src = conf.my_address;
        if(p->payload[0] == 100 && p->payload[1] == 100){
		set_payload_at(r, 2, 117);
	}
	else
        set_payload_at(r, 0, get_payload_at(p,13));
        set_payload_at(r, 1, get_payload_at(p,14));
	set_payload_at(r, 2, 117);
        set_payload_at(r, 3, get_payload_at(p,16));
        set_payload_at(r, 4, get_payload_at(p,17));
	rf_unicast_send(r);
      }*/
      //print_packet_uart(p);
	send_request_to_controller(p);
#else

    p->header.nxh = conf.nxh_vs_sink;
    rf_unicast_send(p);
#endif
  }

/*----------------------------------------------------------------------------*/
  void
  handle_response(packet_t* p)
  {
    if (is_my_address(&(p->header.dst)))
    {    
      entry_t* e = get_entry_from_array(p->payload, p->header.len - PLD_INDEX);
      if (e != NULL)
      {
        add_entry(e);
      }
      packet_deallocate(p);
    } else {
      match_packet(p);
    } 
  }
/*----------------------------------------------------------------------------*/
  void
  handle_open_path(packet_t* p)
  {
    int i;
    
    
    uint8_t n_windows = get_payload_at(p,OPEN_PATH_WINDOWS_INDEX);
    uint8_t start = n_windows*WINDOW_SIZE + 1;
    uint8_t path_len = (p->header.len - (start + PLD_INDEX))/ADDRESS_LENGTH;
    uint8_t my_index = 0;
    uint8_t my_position = 0;
    uint8_t end = p->header.len - PLD_INDEX;
    
    for (i = start; i < end; i += ADDRESS_LENGTH)
    {
      address_t tmp = get_address_from_array(&(p->payload[i]));
      if (is_my_address(&tmp))
      {
        my_index = i;
        break;
      }
      my_position++;
    }


    if (my_position > 0)
    { 
      uint8_t prev = my_index - ADDRESS_LENGTH;
      uint8_t first = start;
      entry_t* e = create_entry();
      
      window_t* w = create_window();
      w->operation = EQUAL;
      w->size = SIZE_2;
      w->lhs = DST_INDEX;
      w->lhs_location = PACKET;
      w->rhs = MERGE_BYTES(p->payload[first], p->payload[first+1]);
      w->rhs_location = CONST;

      add_window(e,w);
      
      for (i = 0; i<n_windows; ++i)
      {
        add_window(e, get_window_from_array(&(p->payload[i*WINDOW_SIZE + 1])));
      }

      action_t* a = create_action(FORWARD_U, &(p->payload[prev]), ADDRESS_LENGTH); 
      add_action(e,a);

      PRINTF("[PHD]: ");
      print_entry(e);
      PRINTF("\n");
      
      add_entry(e);
    }
   
    if (my_position < path_len-1)
    { 
      uint8_t next = my_index + ADDRESS_LENGTH;
      uint8_t last = end - ADDRESS_LENGTH;
      entry_t* e = create_entry();

      window_t* w = create_window();
      w->operation = EQUAL;
      w->size = SIZE_2;
      w->lhs = DST_INDEX;
      w->lhs_location = PACKET;
      w->rhs = MERGE_BYTES(p->payload[last], p->payload[last+1]);
      w->rhs_location = CONST;

      add_window(e,w);
      
      for (i = 0; i<n_windows; ++i)
      {
        add_window(e, get_window_from_array(&(p->payload[i*WINDOW_SIZE + 1])));
      }

      action_t* a = create_action(FORWARD_U, &(p->payload[next]), ADDRESS_LENGTH);
      add_action(e,a);

      PRINTF("[PHD]: ");
      print_entry(e);
      PRINTF("\n");
      
      add_entry(e);

      address_t next_address = get_address_from_array(&(p->payload[next]));
      p->header.nxh = next_address;
      p->header.dst = next_address;
      rf_unicast_send(p);
    }

    if (my_position == path_len-1){
      packet_deallocate(p);
    }
  }
/*----------------------------------------------------------------------------*/
  void
  handle_config(packet_t* p)
  {
    //Million Added add entry to flow table and display flow table
    if (is_my_address(&(p->header.dst))){
	    if(p->payload[2] == 114 && p->payload[3] == 102){ //rf - remove flowtable
		remove_flowtable();
	    }
	    else if(p->payload[2] == 115 && p->payload[3] == 102){ //sf - show flowtable
                PRINTF("Flow Table\n");
		print_flowtable();
            }
	    else if(p->payload[2] == 116 && p->payload[3] == 102 && p->payload[4] == 114){ //tfr - turn off radio
		NETSTACK_MAC.off(0);
		PRINTF("Radio Turned Off\n");
	    }
	    else if(p->payload[2] == 116 && p->payload[3] == 111 && p->payload[4] == 114){ //tor - turn on radio
		NETSTACK_MAC.on();
		PRINTF("Radio Turned On\n");
	    }
            else{
		PRINTF("Flow Table - Before\n");
		print_flowtable();
		entry_t* e = create_entry();
		//entry_t* e2 = create_entry();
		action_t* a;
		//action_t* a2;
	    //Million if Node 1, 2, 3, specify address accordingly
	   	 uint8_t addr[ADDRESS_LENGTH];
		 uint8_t addr2[ADDRESS_LENGTH];
		    /* if(p->payload[3] == 49 && p->payload[4] == 49){ //'1'
		    	addr[0] = 2;
	        	addr[1] = 0;
		    }
		    else if(p->payload[3] == 50 && p->payload[4] == 50){ //'2'
	        	addr[0] = 3;
		        addr[1] = 0;
		    }
		    else if(p->payload[3] == 51 && p->payload[4] == 51){//'3'
		        addr[0] = 4;
	        	addr[1] = 0;
		    }
		    else if(p->payload[3] == 52 && p->payload[4] == 52){//'4'
	        	addr[0] = 5;
		        addr[1] = 0;
		    }
		    else if(p->payload[3] == 53 && p->payload[4] == 53){//'5'
		        addr[0] = 6;
        		addr[1] = 0;
		    }
		    else if(p->payload[3] == 54 && p->payload[4] == 54){//'6'
                	addr[0] = 7;
	                addr[1] = 0;
        	    }
		    else if(p->payload[3] == 55 && p->payload[4] == 55){//'7'
        	        addr[0] = 8;
                	addr[1] = 0;
	            }
		    else if(p->payload[3] == 56 && p->payload[4] == 56){//'8'
                	addr[0] = 9;
	                addr[1] = 0;
        	    }
		    else if(p->payload[3] == 57 && p->payload[4] == 57){//'9'
        	        addr[0] = 10;
                	addr[1] = 0;
	            }
		    /*else if(p->payload[3] == 49 && p->payload[4] == 48){//'10'
                	addr[0] = 11;
	                addr[1] = 0;
	            }*/
			addr[0] = ((p->payload[3] % 10) + 3) % 10;
			addr[1] = 0;
			addr2[0] = ((p->payload[4] % 10) + 3) % 10;
			addr2[1] = 0;
		    if(p->payload[2] == 117){ //'u'
			//a = create_action(FORWARD_U, &(p->payload[0]), ADDRESS_LENGTH);
			if(p->payload[3] != p->payload[4]){
		                a = create_action(FORWARD_U, &(addr2[0]), ADDRESS_LENGTH); 
			} 
			else{
				a = create_action(FORWARD_U, &(addr[0]), ADDRESS_LENGTH);
			}				
			add_action(e,a);
			//Million ... window added
			window_t* w = create_window();
			w->operation = EQUAL;
			w->size = SIZE_2;
			w->lhs = DST_INDEX;
			//w->lhs = TYP_INDEX;
			w->lhs_location = PACKET;
			w->rhs = MERGE_BYTES(addr[0], addr[1]);
			w->rhs_location = CONST;
			add_window(e,w);
			PRINTF("This Entry to be added to flowtable\n");
			print_entry(e);
			add_entry(e);
			PRINTF("Flow Table - After\n");
			print_flowtable();
		    }
		    else if(p->payload[2] == 98){ //'b'
			uint8_t newaddr[ADDRESS_LENGTH];
	  		newaddr[0] = newaddr[1] = 255;
			a = create_action(FORWARD_B, &(newaddr[0]), ADDRESS_LENGTH);
		    }
	  //  else if(p->payload[2] == 97) //'a'
	//	a = create_action(ASK, &(addr[0]), ADDRESS_LENGTH);
	    //else
	 	//a = create_action(DROP, &(addr[0]), ADDRESS_LENGTH);
            }
	    packet_deallocate(p);
	    
	}
	else
		match_packet(p);
    //Original Code from SDN-WISE
/*    if (is_my_address(&(p->header.dst)))
    {    
#if SINK
      if (!is_my_address(&(p->header.src))){
        print_packet_uart(p);
      } else {
#endif
      uint8_t i = 0;
      uint8_t id = p->payload[i] & 127;
      if ((p->payload[i] & 128) == CNF_READ)
      {
        //READ
        switch (id)
        {
          // TODO
          case RESET:
          case GET_ALIAS:
          case GET_RULE:
          case GET_FUNCTION:
          break;


          case MY_NET:
          case MY_ADDRESS:
          case PACKET_TTL:
          case RSSI_MIN:
          case BEACON_PERIOD:
          case REPORT_PERIOD:
          case RULE_TTL:
          // TODO check payload size
          if (conf_size[id] == 1){
            memcpy(&(p->payload[i+1]), conf_ptr[id], conf_size[id]);
          } else if (conf_size[id] == 2) {
            uint16_t value = *((uint16_t*)conf_ptr[id]);
            p->payload[i+1] = value >> 8;
            p->payload[i+2] = value & 0xFF; 
          }
          break;

          default:
          break;
        }
        swap_addresses(&(p->header.src),&(p->header.dst));
        p->header.len += conf_size[id];
        match_packet(p);
      } else {
        //WRITE
        switch (id)
        {
          // TODO
          case ADD_ALIAS:
          case REM_ALIAS:
          case ADD_RULE:
          case REM_RULE:
          case ADD_FUNCTION:
          case REM_FUNCTION:
          break;

          case RESET:
          watchdog_reboot();
          break;

          case MY_NET:
          case MY_ADDRESS:
          case PACKET_TTL:
          case RSSI_MIN:
          case BEACON_PERIOD:
          case REPORT_PERIOD:
          case RULE_TTL:
          if (conf_size[id] == 1){
            memcpy((uint8_t*)conf_ptr[id], &(p->payload[i+1]), conf_size[id]);
          } else if (conf_size[id] == 2) {
            uint16_t h = p->payload[i+1] << 8;
            uint16_t l = p->payload[i+2];            
            *((uint16_t*)conf_ptr[id]) = h + l;
          }
          break;

          default:
          break;
        }
        packet_deallocate(p);
      }
#if SINK
    }
#endif      
    }
    else {
      match_packet(p);
    }*/   
  }
/*----------------------------------------------------------------------------*/
  void 
  test_handle_open_path(void)
  {
    uint8_t array[19] = {
      1, 19, 0, 1, 0, 2, 5, 100, 0, 1, 0, 0, 1, 0, 2, 0, 3, 0, 4,
    };

    packet_t* p = get_packet_from_array(array);
    handle_open_path(p);
  }
/*----------------------------------------------------------------------------*/
/** @} */
