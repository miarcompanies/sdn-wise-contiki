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
 *         SDN-WISE Packet buffer.
 * \author
 *         Sebastiano Milardo <s.milardo@hotmail.it>
 */

/**
 * \addtogroup sdn-wise
 * @{
 */

#include <string.h>
#include <stdio.h>
#include "lib/memb.h"
#include "lib/list.h"

#include "packet-buffer.h"
#include "address.h"

#define MAX_TTL   100

#define DEBUG 1
#if DEBUG && (!SINK || DEBUG_SINK)
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*----------------------------------------------------------------------------*/
  MEMB(packets_memb, packet_t, 4);
/*----------------------------------------------------------------------------*/
  static packet_t * packet_allocate(void);
/*----------------------------------------------------------------------------*/
  void print_report_data(uint8_t a, uint8_t b, uint8_t c, uint8_t d){
    /*putchar(68);
    putchar(35);
    putchar(77);
    putchar(105);
    putchar(108);
    putchar(108);
    putchar(105);
    putchar(111);
    putchar(110);
    putchar(10);*/
    printf("D#Million: Data Command d%u%u%u%u\n", a,b,c,d);
  }
  void print_report_config(uint8_t a, uint8_t b, uint8_t c, uint8_t d){
    /*putchar(67);
    putchar(35);
    putchar(77);
    putchar(105);
    putchar(108);
    putchar(108);
    putchar(105);
    putchar(111);
    putchar(110);
    putchar(10);*/
    printf("C#Million: Config Command %u%uu%u%u\n", a,b,c,d);
  }
  void 
  print_packet_uart(packet_t* p)
  {
    uint16_t i = 0;
    //Million Commented out all except deallocate
/*#if !COOJA
    putchar(122);
#endif
    uint8_t* tmp = (uint8_t*)p;
    for (i = 0; i< p->header.len; ++i){
      putchar(tmp[i]);
    }
#if !COOJA
    putchar(126);
    putchar('\n');
#endif */
    packet_deallocate(p);
  }
  void
  send_report_to_controller(packet_t* p)
  {
	PRINTF("Inside Send_report_to_controller function\n");
	printf("Report:\n");
        uint8_t len = p->payload[2];
	uint8_t reportlen = len*3 + 2;
	uint8_t report[reportlen]; 
	int k=0;
	report[0] = p->header.src.u8[0];
	report[1] = p->header.src.u8[1];
        for(k=2;k<reportlen;k++){
		report[k] = p->payload[k+1];
	}
	int j = 0;
	printf("%d",report[j]);
	for(j=1;j<reportlen;j++){
		printf(",");
		printf("%d",report[j]);
	}
	printf("\n");
	packet_deallocate(p);
  }
  void
  send_request_to_controller(packet_t* p)
  {
        PRINTF("Inside Send_request_to_controller function\n");
        printf("Request:\n");
        uint8_t request[4];
        request[0] = p->header.src.u8[0];
        request[1] = p->header.src.u8[1];
        request[2] = p->payload[5];
	request[3] = p->payload[6];
	int j=0;
        printf("%d",request[j]);
        for(j=1;j<4;j++){
                printf(",");
                printf("%d",request[j]);
        }
        printf("\n");
        packet_deallocate(p);
  }
/*----------------------------------------------------------------------------*/
  void 
  print_packet(packet_t* p)
  {
    uint16_t i = 0;
    PRINTF("Network ID: %d Packet Length: %d ", p->header.net, p->header.len);
    PRINTF("Packet Dst:");
    print_address(&(p->header.dst));
    PRINTF("Packet Src:");
    print_address(&(p->header.src));
    PRINTF("Pkt Type: %d TTL: %d ", p->header.typ, p->header.ttl);
    PRINTF("Next Hop: ");
    print_address(&(p->header.nxh));
    PRINTF("Payload: ");
    for (i=0; i < (p->header.len - PLD_INDEX); ++i){
      PRINTF("%d ",get_payload_at(p,i));
    }
  }
/*----------------------------------------------------------------------------*/
  static packet_t *
  packet_allocate(void)
  {
    packet_t *p = NULL;
    p = memb_alloc(&packets_memb);
    if(p == NULL) {
      PRINTF("[PBF]: Failed to allocate a packet\n");
    }
    return p;
  }
/*----------------------------------------------------------------------------*/
  void
  packet_deallocate(packet_t* p)
  {
    int res = memb_free(&packets_memb, p); 
    if (res !=0){
      PRINTF("[FLT]: Failed to deallocate a packet. Reference count: %d\n",res);
    }
  }
/*----------------------------------------------------------------------------*/
  packet_t* 
  create_packet_payload(uint8_t net, address_t* dst, address_t* src, 
    packet_type_t typ, address_t* nxh, uint8_t* payload, uint8_t len)
  {
    packet_t* p = create_packet(net, dst, src, typ, nxh);
    if (p != NULL){
      uint8_t i;

      for (i = 0; i < len; ++i){
        set_payload_at(p, i, payload[i]);  
      }
    }
    return p;
  }
/*----------------------------------------------------------------------------*/
  packet_t* 
  get_packet_from_array(uint8_t* array)
  {
  // TODO fragmentation
    address_t dst = get_address_from_array(&array[DST_INDEX]);
    address_t src = get_address_from_array(&array[SRC_INDEX]);
    address_t nxh = get_address_from_array(&array[NXH_INDEX]);
    packet_t* p = create_packet_payload(array[NET_INDEX], &dst, &src, 
      array[TYP_INDEX], &nxh, &array[PLD_INDEX], array[LEN_INDEX] - PLD_INDEX);
    return p;
  }
/*----------------------------------------------------------------------------*/
  uint8_t 
  get_payload_at(packet_t* p, uint8_t index)
  {
    if (index < MAX_PACKET_LENGTH){
      return p->payload[index];
    } else {
      return 0;
    }
  }
/*----------------------------------------------------------------------------*/
  void 
  set_payload_at(packet_t* p, uint8_t index, uint8_t value)
  {
    if (index < MAX_PACKET_LENGTH){
      p->payload[index] = value;
      if (index + PLD_INDEX + 1 > p->header.len){
        p->header.len = index + PLD_INDEX + 1;
      }
    }
  }
/*----------------------------------------------------------------------------*/
  void 
  restore_ttl(packet_t* p)
  {
    p->header.ttl = MAX_TTL;
  }
/*----------------------------------------------------------------------------*/
  packet_t* 
  create_packet_empty(void)
  {
    packet_t* p = packet_allocate();
    if (p != NULL){
      memset(&(p->header), 0, sizeof(p->header));
      memset(&(p->info), 0, sizeof(p->info));
      restore_ttl(p);
    } 
    return p;
  }
/*----------------------------------------------------------------------------*/
  packet_t* 
  create_packet(uint8_t net, address_t* dst, address_t* src, packet_type_t typ, 
    address_t* nxh)
  {
    packet_t* p = packet_allocate();
    if (p != NULL){
      memset(&(p->header), 0, sizeof(p->header));
      memset(&(p->info), 0, sizeof(p->info));
      p->header.net=net;
      p->header.dst=*dst;
      p->header.src=*src;
      p->header.typ=typ;
      p->header.nxh=*nxh;
      restore_ttl(p);
    } 
    return p;
  }
/*----------------------------------------------------------------------------*/
  void 
  packet_buffer_init(void)
  {
    memb_init(&packets_memb);
  }

/*----------------------------------------------------------------------------*/
  void 
  test_packet_buffer(void)
  {
    uint8_t array[73] = {1, 73, 0, 0, 0, 2, 4, 100, 0, 0, 20, 18, 0, 6, 0, 10, 
      18, 0, 50, 0, 1, 90, 0, 10, 0, 1, 122, 0, 12, 0, 5, 1, 4, 8, 6, 2, 0, 10, 
      0, 40, 0, 0, 8, 5, 1, 0, 0, 0, 0, 0, 0, 1, 3, 3, 2, 255, 255, 3, 1, 0, 3, 
      1, 7, 8, 6, 132, 0, 11, 0, 12, 0, 13, 254};

      packet_t* second = get_packet_from_array(array);
      print_packet(second);
  }
/*----------------------------------------------------------------------------*/
/** @} */
