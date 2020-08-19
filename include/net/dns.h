#pragma once
// an DNS packet (comes after an UDP header).
struct dns {
  uint16_t id;  // request ID

  uint8_t rd: 1;  // recursion desired
  uint8_t tc: 1;  // truncated
  uint8_t aa: 1;  // authoritive
  uint8_t opcode: 4; 
  uint8_t qr: 1;  // query/response
  uint8_t rcode: 4; // response code
  uint8_t cd: 1;  // checking disabled
  uint8_t ad: 1;  // authenticated data
  uint8_t z:  1;  
  uint8_t ra: 1;  // recursion available
  
  uint16_t qdcount; // number of question entries
  uint16_t ancount; // number of resource records in answer section
  uint16_t nscount; // number of NS resource records in authority section
  uint16_t arcount; // number of resource records in additional records
} __attribute__((packed));

struct dns_question {
  uint16_t qtype;
  uint16_t qclass;
} __attribute__((packed));
  
#define ARECORD (0x0001)
#define QCLASS  (0x0001)

struct dns_data {
  uint16_t type;
  uint16_t class;
  uint32_t ttl;
  uint16_t len;
} __attribute__((packed));
