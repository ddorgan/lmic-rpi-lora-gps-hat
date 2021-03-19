/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include "lmic.h"
#include "debug.h"

//////////////////////////////////////////////////
// CONFIGURATION (FOR APPLICATION CALLBACKS BELOW)
//////////////////////////////////////////////////


// application router ID (LSBF)
static const u1_t APPEUI[8]  = { 0x02, 0x00, 0x00, 0x00, 0x00, 0xEE, 0xFF, 0xC0 };

// unique device ID (LSBF)
static const u1_t DEVEUI[8]  = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };

// device-specific AES key (derived from device EUI)
static const u1_t DEVKEY[16] = { 0xAB, 0x89, 0xEF, 0xCD, 0x23, 0x01, 0x67, 0x45, 0x54, 0x76, 0x10, 0x32, 0xDC, 0xFE, 0x98, 0xBA };

//////////////////////////////////////////////////
// APPLICATION CALLBACKS
//////////////////////////////////////////////////

// provide application router ID (8 bytes, LSBF)
void os_getArtEui (u1_t* buf) {
    memcpy(buf, APPEUI, 8);
}

// provide device ID (8 bytes, LSBF)
void os_getDevEui (u1_t* buf) {
    memcpy(buf, DEVEUI, 8);
}

// provide device key (16 bytes)
void os_getDevKey (u1_t* buf) {
    memcpy(buf, DEVKEY, 16);
}


//////////////////////////////////////////////////
// MAIN - INITIALIZATION AND STARTUP
//////////////////////////////////////////////////

// initial job
static void initfunc (osjob_t* j) {
    // reset MAC state
    LMIC_reset();
    // start joining
    LMIC_startJoining();
    LMIC_setLinkCheckMode(0);
    // init done - onEvent() callback will be invoked...
}

// scheduled job
static void scheduledfunc(osjob_t* j)
{
    static unsigned long sec_counter = 0;
    printf("%d seconds\n", sec_counter++);
    unsigned char mydata1[103] = "3d0284d43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d017ae63a5e5472f8018e3336d93f03fe";
    unsigned char mydata2[103] = "848229657733d8ee7d0bc08517fd454c5c042292fe74fa944096c3b47a7b1803de3164b84cfe7b4fce6ed1f8b65c30cb817502";
    unsigned char mydata3[87] = "000005038eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a480b00b01723010a";

    LMIC.bands[BAND_MILLI].avail =
    LMIC.bands[BAND_CENTI].avail =
    LMIC.bands[BAND_DECI ].avail = os_getTime();
    if(sec_counter==4) {
       printf(">> send mydata1\n");
       LMIC_setTxData2(1, mydata1, sizeof(mydata1)-1, 0);
    }
    else if(sec_counter==5) {
       printf(">> send mydata2\n");
       LMIC_setTxData2(1, mydata2, sizeof(mydata2)-1, 0);
    }
    else if(sec_counter==6) {
       printf(">> send mydata3\n");
       LMIC_setTxData2(1, mydata3, sizeof(mydata3)-1, 0);
    }
    else {
       printf(">> skip sending\n");
    }
    os_setTimedCallback(j, os_getTime()+sec2osticks(4), scheduledfunc);
}


// application entry point
int main () {
    osjob_t initjob;
    osjob_t scheduledjob;

    // initialize runtime env
    os_init();
    // initialize debug library
    debug_init();
    // setup initial job
    os_setCallback(&initjob, initfunc);
    os_setCallback(&scheduledjob, scheduledfunc);
    // execute scheduled jobs and events
    os_runloop();
    // (not reached)
    return 0;
}


//////////////////////////////////////////////////
// LMIC EVENT CALLBACK
//////////////////////////////////////////////////

void onEvent (ev_t ev) {
    debug_event(ev);

    switch(ev) {
   
      // network joined, session established
      case EV_JOINED:
          debug_val("netid = ", LMIC.netid);
          goto tx;
        
      // scheduled data sent (optionally data received)
      case EV_TXCOMPLETE:
          if(LMIC.dataLen) { // data received in rx slot after tx
              debug_buf(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
          }
        tx:
	  // immediately prepare next transmission
	  LMIC.frame[0] = LMIC.snr;
          printf(">>>>> in _tx\n");
	  // schedule transmission (port 1, datalen 1, no ack requested)
	  ////LMIC_setTxData2(1, LMIC.frame, 1, 0);
          // (will be sent as soon as duty cycle permits)
	  break;
    }
}
