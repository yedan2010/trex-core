/*
 Hanoh Haim
 Cisco Systems, Inc.
*/

/*
Copyright (c) 2015-2015 Cisco Systems, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#include "bp_sim.h"
#include "os_time.h"


#include <common/arg/SimpleGlob.h>
#include <common/arg/SimpleOpt.h>
#include <stateless/cp/trex_stateless.h>


// An enum for all the option types
enum { OPT_HELP, OPT_CFG, OPT_NODE_DUMP, OP_STATS,
          OPT_FILE_OUT, OPT_UT, OPT_PCAP, OPT_IPV6, OPT_MAC_FILE};
      

/* these are the argument types:
   SO_NONE --    no argument needed
   SO_REQ_SEP -- single required argument
   SO_MULTI --   multiple arguments needed
*/
static CSimpleOpt::SOption parser_options[] =
{
    { OPT_HELP,     "-?",           SO_NONE   },
    { OPT_HELP,     "-h",           SO_NONE   },
    { OPT_HELP,     "--help",       SO_NONE   },
    { OPT_UT,       "--ut",         SO_NONE   },
    { OP_STATS,       "-s",         SO_NONE   },
    { OPT_CFG,      "-f",           SO_REQ_SEP},
    { OPT_MAC_FILE, "--mac",        SO_REQ_SEP},
    { OPT_FILE_OUT , "-o",          SO_REQ_SEP },
    { OPT_NODE_DUMP , "-v",         SO_REQ_SEP },
    { OPT_PCAP,       "--pcap",       SO_NONE   },
    { OPT_IPV6,       "--ipv6",       SO_NONE   },

    
    SO_END_OF_OPTIONS
};




static int usage(){

    printf(" Usage: bp_sim [OPTION] -f cfg.yaml -o outfile.erf   \n");
    printf(" \n");
    printf(" \n");
    printf(" options \n");
    printf(" -v  [1-3]   verbose mode  \n");
    printf("      1    show only stats  \n");
    printf("      2    run preview do not write to file  \n");
    printf("      3    run preview write stats file  \n");
    printf("  Note in case of verbose mode you don't need to add the output file \n");
    printf("   \n");
    printf("  Warning : This program can generate huge-files (TB ) watch out! try this only on local drive \n");
    printf(" \n");
    printf(" --pcap  export the file in pcap mode \n");
    printf(" Examples: ");
    printf("  1) preview show csv stats \n");
    printf("  #>bp_sim -f cfg.yaml -v 1 \n");
    printf("  \n ");
    printf("  2) more detail preview preview show csv stats \n");
    printf("  #>bp_sim -f cfg.yaml -v 2 \n");
    printf("  \n ");
    printf("  3) more detail preview plus stats  \n");
    printf("  #>bp_sim -f cfg.yaml -v 3 \n");
    printf("  \n ");
    printf("  4) do the job  ! \n");
    printf("  #>bp_sim -f cfg.yaml -o outfile.erf \n");
    printf("\n");
    printf("\n");
    printf(" Copyright (C) 2015 by hhaim Cisco-System for IL dev-test \n");
    printf(" version : 1.0 beta  \n");
    return (0);
}

int gtest_main(int argc, char **argv) ;

static int parse_options(int argc, char *argv[], CParserOption* po, bool & is_gtest ) {
     CSimpleOpt args(argc, argv, parser_options);

     int a=0;
     int node_dump=0;
     po->preview.clean();
     po->preview.setFileWrite(true);

     while ( args.Next() ){
        if (args.LastError() == SO_SUCCESS) {
            switch (args.OptionId()) {
            case OPT_UT :
                is_gtest=true;
                return (0);
                break;
            case OPT_HELP: 
                usage();
                return -1;
            case OPT_CFG:
                po->cfg_file = args.OptionArg();
                break;
            case OPT_MAC_FILE:
                po->mac_file = args.OptionArg();
                break;
            case OPT_FILE_OUT:
                po->out_file = args.OptionArg();
                break;
            case OPT_IPV6:
                po->preview.set_ipv6_mode_enable(true);
                break;
            case OPT_NODE_DUMP:
                a=atoi(args.OptionArg());
                node_dump=1;
                po->preview.setFileWrite(false);
                break;
            case OPT_PCAP:
                po->preview.set_pcap_mode_enable(true);
                break;
            default:
                usage();
                return -1;
                break;
            } // End of switch
         }// End of IF
        else {
            usage();
            return -1;
        }
     } // End of while
     

    if ((po->cfg_file =="") ) {
         printf("Invalid combination of parameters you must add -f with configuration file \n");
         usage();
         return -1;
     }

    if ( node_dump ){
        po->preview.setVMode(a);
    }else{
        if  (po->out_file=="" ){
         printf("Invalid combination of parameters you must give output file iwth -o  \n");
         usage();
         return -1;
        }
    }
    return 0;
}

int cores=1;

/*

int curent_time(){

	time_init();

	int i;
	for (i=0; i<100000000; i++){
	  now=now_sec();
	}
	return (0);
}*/

#ifdef LINUX



#include <pthread.h>

struct per_thread_t {
    pthread_t  tid;
};

#define MAX_THREADS 200
static per_thread_t  tr_info[MAX_THREADS];


//////////////

struct test_t_info1 {
    CPreviewMode *            preview_info;
    CFlowGenListPerThread   * thread_info;
    uint32_t                  thread_id;
};

void * thread_task(void *info){

    test_t_info1 * obj =(test_t_info1 *)info;

    CFlowGenListPerThread   * lpt=obj->thread_info;

    printf("start thread %d \n",obj->thread_id);
    //delay(obj->thread_id *3000);
    printf("-->start thread %d \n",obj->thread_id);
    if (1/*obj->thread_id ==3*/) {

        char buf[100];
        sprintf(buf,"my%d.erf",obj->thread_id);
        lpt->start_generate_stateful(buf,*obj->preview_info);
        lpt->m_node_gen.DumpHist(stdout);
        printf("end thread %d \n",obj->thread_id);
    }

    return (NULL);
}


void test_load_list_of_cap_files_linux(CParserOption * op){

    CFlowGenList fl;
    //CNullIF erf_vif;
    //CErfIF erf_vif;

    fl.Create();

    fl.load_from_yaml(op->cfg_file,cores);
    fl.DumpPktSize();

    
    fl.generate_p_thread_info(cores);
    CFlowGenListPerThread   * lpt;

    /* set the ERF file */
    //fl.set_vif_all(&erf_vif);

    int i;
    for (i=0; i<cores; i++) {
        lpt=fl.m_threads_info[i];
        test_t_info1 * obj = new test_t_info1();
        obj->preview_info =&op->preview;
        obj->thread_info  = fl.m_threads_info[i];
        obj->thread_id    = i;
        CNullIF * erf_vif = new CNullIF();
        //CErfIF  * erf_vif = new CErfIF();

        lpt->set_vif(erf_vif);

        assert(pthread_create( &tr_info[i].tid, NULL, thread_task, obj)==0);
    }

    for (i=0; i<cores; i++) {
        /* wait for all of them to stop */
       assert(pthread_join((pthread_t)tr_info[i].tid,NULL )==0);
    }

    printf("compare files \n");
    for (i=1; i<cores; i++) {

        CErfCmp cmp;
        char buf[100];
        sprintf(buf,"my%d.erf",i);
        char buf1[100];
        sprintf(buf1,"my%d.erf",0);
        if ( cmp.compare(std::string(buf),std::string(buf1)) != true ) {
            printf(" ERROR cap file is not ex !! \n");
            assert(0);
        }
        printf(" thread %d is ok \n",i);
    }

    fl.Delete();
}


#endif

/*************************************************************/
void test_load_list_of_cap_files(CParserOption * op){

    CFlowGenList fl;
    CNullIF erf_vif;

    fl.Create();

    #define NUM 1

    fl.load_from_yaml(op->cfg_file,NUM);
    fl.DumpPktSize();
    
    
    fl.generate_p_thread_info(NUM);
    CFlowGenListPerThread   * lpt;

    /* set the ERF file */
    //fl.set_vif_all(&erf_vif);

    int i;
    for (i=0; i<NUM; i++) {
        lpt=fl.m_threads_info[i];
        char buf[100];
        sprintf(buf,"my%d.erf",i);
        lpt->start_generate_stateful(buf,op->preview);
        lpt->m_node_gen.DumpHist(stdout);
    }
    //sprintf(buf,"my%d.erf",7);
    //lpt=fl.m_threads_info[7];

    //fl.Dump(stdout);
    fl.Delete();
}

int load_list_of_cap_files(CParserOption * op){
    CFlowGenList fl;
    fl.Create();
    fl.load_from_yaml(op->cfg_file,1);
    if ( op->preview.getVMode() >0 ) {
        fl.DumpCsv(stdout);
    }
    uint32_t start=    os_get_time_msec();

    CErfIF erf_vif;
    //CNullIF erf_vif;

    fl.generate_p_thread_info(1);
    CFlowGenListPerThread   * lpt;
    lpt=fl.m_threads_info[0];
    lpt->set_vif(&erf_vif);

    if ( (op->preview.getVMode() >1)  || op->preview.getFileWrite() ) {
        lpt->start_generate_stateful(op->out_file,op->preview);
    }

    lpt->m_node_gen.DumpHist(stdout);

    uint32_t stop=    os_get_time_msec();
    printf(" d time = %ul %ul \n",stop-start,os_get_time_freq());
    fl.Delete();
    return (0);
}


int test_dns(){

    time_init();
    CGlobalInfo::init_pools(1000);

    CParserOption po ;

    //po.cfg_file = "cap2/dns.yaml";
    //po.cfg_file = "cap2/sfr3.yaml";
    po.cfg_file = "cap2/sfr.yaml";

    po.preview.setVMode(0);
    po.preview.setFileWrite(true);
    #ifdef LINUX
      test_load_list_of_cap_files_linux(&po);
    #else
      test_load_list_of_cap_files(&po);
    #endif
    return (0);
}

void test_pkt_mbuf(void);

void test_compare_files(void);

#if 0
static int b=0;
static int c=0;
static int d=0;

int test_instructions(){
    int i;
    for (i=0; i<100000;i++) {
        b+=b+1;
        c+=+b+c+1;
        d+=+(b*2+1);
    }
    return (b+c+d);
}

#include <valgrind/callgrind.h>
#endif


void update_tcp_seq_num(CCapFileFlowInfo * obj,
                        int pkt_id,
                        int size_change){
    CFlowPktInfo * pkt=obj->GetPacket(pkt_id);
    if ( pkt->m_pkt_indication.m_desc.IsUdp() ){
        /* nothing to do */
        return;
    }

    bool o_init=pkt->m_pkt_indication.m_desc.IsInitSide();
    TCPHeader * tcp ;
    int s= (int)obj->Size();
    int i;

    for (i=pkt_id+1; i<s; i++) {

        pkt=obj->GetPacket(i);
        tcp=pkt->m_pkt_indication.l4.m_tcp;
        bool init=pkt->m_pkt_indication.m_desc.IsInitSide();
        if (init == o_init) {
            /* same dir update the seq number */
            tcp->setSeqNumber    (tcp->getSeqNumber    ()+size_change);

        }else{
            /* update the ack number */
            tcp->setAckNumber    (tcp->getAckNumber    ()+size_change);
        }
    }
}



void change_pkt_len(CCapFileFlowInfo * obj,int pkt_id, int size ){
    CFlowPktInfo * pkt=obj->GetPacket(pkt_id);

    /* enlarge the packet size by 9 */

    char * p=pkt->m_packet->append(size);
    /* set it to 0xaa*/
    memmove(p+size-4,p-4,4); /* CRCbytes */
    memset(p-4,0x0a,size);

    /* refresh the pointers */
    pkt->m_pkt_indication.RefreshPointers();

    IPHeader       * ipv4 = pkt->m_pkt_indication.l3.m_ipv4;
    ipv4->updateTotalLength	(ipv4->getTotalLength()+size );

    /* update seq numbers if needed */
    update_tcp_seq_num(obj,pkt_id,size);
}

void dump_tcp_seq_num_(CCapFileFlowInfo * obj){
    int s= (int)obj->Size();
    int i;
    uint32_t i_seq;
    uint32_t r_seq;

    CFlowPktInfo * pkt=obj->GetPacket(0);
    TCPHeader * tcp = pkt->m_pkt_indication.l4.m_tcp;
    i_seq=tcp->getSeqNumber    ();

    pkt=obj->GetPacket(1);
    tcp = pkt->m_pkt_indication.l4.m_tcp;
    r_seq=tcp->getSeqNumber    ();

    for (i=2; i<s; i++) {
        uint32_t seq;
        uint32_t ack;

        pkt=obj->GetPacket(i);
        tcp=pkt->m_pkt_indication.l4.m_tcp;
        bool init=pkt->m_pkt_indication.m_desc.IsInitSide();
        seq=tcp->getSeqNumber    ();
        ack=tcp->getAckNumber    ();
        if (init) {
            seq=seq-i_seq;
            ack=ack-r_seq;
        }else{
            seq=seq-r_seq;
            ack=ack-i_seq;
        }
        printf(" %4d ",i);
        if (!init) {
            printf("                             ");
        }
        printf("  %s   seq: %4d   ack : %4d   \n",init?"I":"R",seq,ack);
    }
}


int manipolate_capfile() {
    time_init();
    CGlobalInfo::init_pools(1000);

    CCapFileFlowInfo flow_info;
    flow_info.Create();

    flow_info.load_cap_file("avl/delay_10_rtsp_0.pcap",0,0);

    change_pkt_len(&flow_info,4-1 ,6);
    change_pkt_len(&flow_info,5-1 ,6);
    change_pkt_len(&flow_info,6-1 ,6+2);
    change_pkt_len(&flow_info,7-1 ,4);
    change_pkt_len(&flow_info,8-1 ,6+2);
    change_pkt_len(&flow_info,9-1 ,4);
    change_pkt_len(&flow_info,10-1,6);
    change_pkt_len(&flow_info,13-1,6);
    change_pkt_len(&flow_info,16-1,6);
    change_pkt_len(&flow_info,19-1,6);

    flow_info.save_to_erf("exp/c.pcap",1);
    
    return (1);
}

int manipolate_capfile_sip() {
    time_init();
    CGlobalInfo::init_pools(1000);

    CCapFileFlowInfo flow_info;
    flow_info.Create();

    flow_info.load_cap_file("avl/delay_10_sip_0.pcap",0,0);

    change_pkt_len(&flow_info,1-1 ,6+6);
    change_pkt_len(&flow_info,2-1 ,6+6);

    flow_info.save_to_erf("exp/delay_10_sip_0_fixed.pcap",1);
    
    return (1);
}

int manipolate_capfile_sip1() {
    time_init();
    CGlobalInfo::init_pools(1000);

    CCapFileFlowInfo flow_info;
    flow_info.Create();

    flow_info.load_cap_file("avl/delay_sip_0.pcap",0,0);
    flow_info.GetPacket(1);

    change_pkt_len(&flow_info,1-1 ,6+6+10);

    change_pkt_len(&flow_info,2-1 ,6+6+10);

    flow_info.save_to_erf("exp/delay_sip_0_fixed_1.pcap",1);
    
    return (1);
}


class CMergeCapFileRec {
public:

    CCapFileFlowInfo m_cap;

    int              m_index;
    int              m_limit_number_of_packets; /* limit number of packets */
    bool             m_stop; /* Do we have more packets */

    double           m_offset; /* offset should be positive */
    double           m_start_time;

public:
    bool Create(std::string cap_file,double offset);
    void Delete();
    void IncPacket();
    bool GetCurPacket(double & time);
    CPacketIndication *  GetUpdatedPacket();

    void Dump(FILE *fd,int _id);
};


void CMergeCapFileRec::Dump(FILE *fd,int _id){
    double time = 0.0;
    bool stop=GetCurPacket(time);
    fprintf (fd," id:%2d  stop : %d index:%4d  %3.4f \n",_id,stop?1:0,m_index,time);
}


CPacketIndication *  CMergeCapFileRec::GetUpdatedPacket(){
    double t1;
    assert(GetCurPacket(t1)==false);
    CFlowPktInfo * pkt = m_cap.GetPacket(m_index);
    pkt->m_pkt_indication.m_packet->set_new_time(t1);
    return (&pkt->m_pkt_indication);
}


bool  CMergeCapFileRec::GetCurPacket(double & time){
    if (m_stop) {
        return(true);
    }
    CFlowPktInfo * pkt = m_cap.GetPacket(m_index);
    time= (pkt->m_packet->get_time() -m_start_time + m_offset);
    return (false);
}

void CMergeCapFileRec::IncPacket(){
    m_index++;
    if ( (m_limit_number_of_packets) && (m_index > m_limit_number_of_packets ) ) {
        m_stop=true;
        return;
    }

    if ( m_index == (int)m_cap.Size() ) {
        m_stop=true;
    }
}

void CMergeCapFileRec::Delete(){
    m_cap.Delete();
}

bool CMergeCapFileRec::Create(std::string cap_file,
                              double offset){
   m_cap.Create();
   m_cap.load_cap_file(cap_file,0,0);
   CFlowPktInfo * pkt = m_cap.GetPacket(0);

   m_index=0;
   m_stop=false;
   m_limit_number_of_packets =0;
   m_start_time =     pkt->m_packet->get_time() ;
   m_offset = offset;

   return (true);
}



#define MERGE_CAP_FILES (2)

class CMergeCapFile {
public:
    bool Create();
    void Delete();
    bool run_merge(std::string to_cap_file);
private:
    void append(int _cap_id);

public:
    CMergeCapFileRec m[MERGE_CAP_FILES];
    CCapFileFlowInfo m_results;
};

bool CMergeCapFile::Create(){
    m_results.Create();
    return(true);
}

void CMergeCapFile::Delete(){
    m_results.Delete();
}

void CMergeCapFile::append(int _cap_id){
    CPacketIndication * lp=m[_cap_id].GetUpdatedPacket();
    lp->m_packet->Dump(stdout,0);
    m_results.Append(lp);
}


bool CMergeCapFile::run_merge(std::string to_cap_file){

    int i=0;
    int cnt=0;
    while ( true ) {
        int    min_index=0;
        double min_time;

        fprintf(stdout," --------------\n");
        fprintf(stdout," pkt : %d \n",cnt);
        for (i=0; i<MERGE_CAP_FILES; i++) {
            m[i].Dump(stdout,i);
        }
        fprintf(stdout," --------------\n");

        bool valid = false;
        for (i=0; i<MERGE_CAP_FILES; i++) {
            double t1;
            if ( m[i].GetCurPacket(t1) == false ){
                /* not in stop */
                if (!valid) {
                    min_time  = t1;
                    min_index = i;
                    valid=true;
                }else{
                    if (t1 < min_time) {
                        min_time=t1;
                        min_index = i;
                    }
                }

            }
        }

        /* nothing to do */
        if (valid==false) {
            fprintf(stdout,"nothing to do \n");
            break;
        }

        cnt++;
        fprintf(stdout," choose id %d \n",min_index);
        append(min_index);
        m[min_index].IncPacket();
    };

    m_results.save_to_erf(to_cap_file,1);

    return (true);
}



int merge_3_cap_files() {
    time_init();
    CGlobalInfo::init_pools(1000);

    CMergeCapFile merger;
    merger.Create();
    merger.m[0].Create("exp/c.pcap",0.001);
    merger.m[1].Create("avl/delay_10_rtp_160k_0.pcap",0.31);
    merger.m[2].Create("avl/delay_10_rtp_160k_1.pcap",0.311);

    //merger.m[1].Create("avl/delay_10_rtp_250k_0_0.pcap",0.31);
    //merger.m[1].m_limit_number_of_packets =6;
    //merger.m[2].Create("avl/delay_10_rtp_250k_1_0.pcap",0.311);
    //merger.m[2].m_limit_number_of_packets =6;

    merger.run_merge("exp/delay_10_rtp_160k_full.pcap");

    return (0);
}

int merge_2_cap_files_sip() {
    time_init();
    CGlobalInfo::init_pools(1000);

    CMergeCapFile merger;
    merger.Create();
    merger.m[0].Create("exp/delay_sip_0_fixed_1.pcap",0.001);
    merger.m[1].Create("avl/delay_video_call_rtp_0.pcap",0.51);
    //merger.m[1].m_limit_number_of_packets=7;

    //merger.m[1].Create("avl/delay_10_rtp_250k_0_0.pcap",0.31);
    //merger.m[1].m_limit_number_of_packets =6;
    //merger.m[2].Create("avl/delay_10_rtp_250k_1_0.pcap",0.311);
    //merger.m[2].m_limit_number_of_packets =6;

    merger.run_merge("avl/delay_10_sip_video_call_full.pcap");

    return (0);
}

static TrexStateless *g_trex_stateless;


TrexStateless * get_stateless_obj() {
    return g_trex_stateless;
}

extern "C" const char * get_build_date(void){ 
    return (__DATE__);
}      
 
extern "C" const char * get_build_time(void){ 
    return (__TIME__ );
} 




int main(int argc , char * argv[]){
    int res=0;
    time_init();
    CGlobalInfo::m_socket.Create(0);
    CGlobalInfo::init_pools(1000);
    assert( CMsgIns::Ins()->Create(4) );


    bool is_gtest=false;

    if ( parse_options(argc, argv, &CGlobalInfo::m_options , is_gtest) != 0){
        exit(-1);
    }

    if ( is_gtest ) {
        res = gtest_main(argc, argv);
    }else{
        res = load_list_of_cap_files(&CGlobalInfo::m_options);
    }

    CMsgIns::Ins()->Free();
    CGlobalInfo::free_pools();
    CGlobalInfo::m_socket.Delete();


    return (res);
    
}



