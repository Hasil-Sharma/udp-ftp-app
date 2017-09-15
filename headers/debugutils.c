#include <stdio.h>
#include "debugutils.h"

void debug_print_pkt(struct packet * pkt){
  DEBUGS1("===>Printing Pkt<===");
  debug_print_hdr(&(pkt->hdr));
  DEBUGS1("=>Printing Payload<=");
  DEBUGS1(pkt->payload); 
}

void debug_print_hdr(struct header *hdr){
  DEBUGS1( "=>Printing Header<=");
  DEBUGN("seq_id", hdr->seq_id);
  DEBUGN("offset", hdr->offset);
  DEBUGN("flag", hdr->flag);
  DEBUGN("checksum", hdr->checksum);
}
