#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
static volatile bool force_quit;
static int mac_updating = 1;
#define RTE_LOGTYPE_fwd RTE_LOGTYPE_USER1
#define NB_MBUF   8192
#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN_US 100
#define MEMPOOL_CACHE_SIZE 256
#define RTE_TEST_RX_DESC_DEFAULT 256
#define RTE_TEST_TX_DESC_DEFAULT 512
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;
static struct ether_addr fwd_ports_eth_addr[RTE_MAX_ETHPORTS];
static uint32_t fwd_enabled_port_mask = 0;
static unsigned int fwd_rx_queue_per_lcore = 1;
#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16
struct lcore_queue_conf {
	unsigned n_rx_port;
	unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];
static const struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 0, /**< IP checksum offload disabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 1, /**< CRC stripped by hardware */
	},
};
struct rte_mempool * fwd_pktmbuf_pool = NULL;
struct speedM{
  uint64_t rx;
}__rte_cache_aligned;
struct speedM port_speed[RTE_MAX_ETHPORTS];
struct fwd_port_statistics {
	uint64_t rx;
} __rte_cache_aligned;
struct fwd_port_statistics port_statistics[RTE_MAX_ETHPORTS];
#define MAX_TIMER_PERIOD 86400 /* 1 day max */
#define K 1/* period is 2 seconds */
struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
struct rte_mbuf *m;
int MK=0;
int MKK=0;
int flag=0;
struct timeval tv1,tv2;
static uint64_t timer_period = K; 
static void print_stats(void)
{     	uint64_t v=0;
        uint64_t  total_packets_rx=0;
	uint64_t  total_speed_rx=0;
	unsigned portid;
	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };
	printf("%s%s", clr, topLeft);
	printf("\nPort statistics ====================================");
	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
		if ((fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("\nStatistics for port %u ------------------------------"
			   "\nPackets received: %20"PRIu64  ,   
			   portid, port_statistics[portid].rx
		      );		
		v=8*MK*( port_statistics[portid].rx-port_speed[portid].rx)/(K*1024*1024);
		if((flag==0) &&v > 0){
		    gettimeofday(&tv1,NULL);
		    flag=1;
		}
		if(v>0){
		   gettimeofday(&tv2,NULL);
		}				 
        printf("\nspeed:  %30"PRIu64 ,( port_statistics[portid].rx-port_speed[portid].rx)/K);
	    printf("\nspeed:  %30"PRIu64" Mb/s" ,v);		
		printf("\ntime: %30ld.%ld s",tv2.tv_sec-tv1.tv_sec ,(tv2.tv_usec-tv1.tv_usec)/10000);
		total_packets_rx += port_statistics[portid].rx;
		total_speed_rx+= port_statistics[portid].rx-port_speed[portid].rx;
	}
	printf("\nAggregate statistics ==============================="
		   "\nTotal packets received: %14"PRIu64, total_packets_rx);
	 printf("\ntotal  speed: %24"PRIu64 , total_speed_rx/K);  	
	 printf("\ntotal  speed: %24"PRIu64"Mb/s" , 8*MK*total_speed_rx/(K*1024*1024)); 
	 printf("\ndata  size    %24d",MKK);
	 printf("\n=================================================\n");
}
static void main_loop(void)
{       unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
        unsigned i, j, portid,nb_rx;
	struct lcore_queue_conf *qconf;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S *
			BURST_TX_DRAIN_US;
	prev_tsc = 0;
	timer_tsc = 0;
	lcore_id = rte_lcore_id();
	qconf = &lcore_queue_conf[lcore_id];
	for (i = 0; i < qconf->n_rx_port; i++) {
		portid = qconf->rx_port_list[i];
	}
	while (!force_quit) {
		cur_tsc = rte_rdtsc();
		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > drain_tsc)) {		
			if (timer_period > 0) {			
				timer_tsc += diff_tsc;
				if (unlikely(timer_tsc >= timer_period)) {
					if (lcore_id == rte_get_master_lcore()) {                                                					        			                         
					        print_stats();
						timer_tsc = 0;
					}
				}
			}
			prev_tsc = cur_tsc;
		}
		for (i = 0; i < qconf->n_rx_port; i++) {
                        if(timer_tsc==0){
			    for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
		            if ((fwd_enabled_port_mask & (1 << portid)) == 0)
			      continue;
			    port_speed[portid].rx=port_statistics[portid].rx;
			  }
			}
			portid = qconf->rx_port_list[i];
			nb_rx = rte_eth_rx_burst(portid, 0, pkts_burst, MAX_PKT_BURST);
			port_statistics[portid].rx += nb_rx;
			for (j = 0; j < nb_rx; j++) {
				m = pkts_burst[j];
				//rte_prefetch0(rte_pktmbuf_mtod(m, void *));
				MKK=m->data_len;
				MK=rte_pktmbuf_pkt_len(m)+4;
				rte_pktmbuf_free(m);				
			}
		}
	}
}
static int fwd_launch_one_lcore(__attribute__((unused)) void *dummy)
{       main_loop();
	return 0;
}
static int fwd_parse_portmask(const char *portmask)
{	char *end = NULL;
	unsigned long pm;
	pm = strtoul(portmask, &end, 16);
	if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;
	if (pm == 0)
		return -1;
	return pm;
}
static unsigned int fwd_parse_nqueue(const char *q_arg)
{	char *end = NULL;
	unsigned long n;
	/* parse hexadecimal string */
	n = strtoul(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return 0;
	if (n == 0)
		return 0;
	if (n >= MAX_RX_QUEUE_PER_LCORE)
		return 0;
	return n;
}
static int  fwd_parse_timer_period(const char *q_arg)
{	char *end = NULL;
	int n;
	n = strtol(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;
	if (n >= MAX_TIMER_PERIOD)
		return -1;
	return n;
}
static const char short_options[] ="p:" "q:"  "T:" ;
#define CMD_LINE_OPT_MAC_UPDATING "mac-updating"
#define CMD_LINE_OPT_NO_MAC_UPDATING "no-mac-updating"
enum {CMD_LINE_OPT_MIN_NUM = 256};
static const struct option lgopts[] = {
	{ CMD_LINE_OPT_MAC_UPDATING, no_argument, &mac_updating, 1},
	{ CMD_LINE_OPT_NO_MAC_UPDATING, no_argument, &mac_updating, 0},
	{NULL, 0, 0, 0}
};
static int fwd_parse_args(int argc, char **argv)
{	int opt, ret, timer_secs;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];
	argvopt = argv;
	while ((opt = getopt_long(argc, argvopt, short_options,
				  lgopts, &option_index)) != EOF) {
		switch (opt) {
		case 'p':
			fwd_enabled_port_mask = fwd_parse_portmask(optarg);
			if (fwd_enabled_port_mask == 0) {
				printf("invalid portmask\n");
				return -1;
			}
			break;
		case 'q':
			fwd_rx_queue_per_lcore = fwd_parse_nqueue(optarg);
			if (fwd_rx_queue_per_lcore == 0) {
				printf("invalid queue number\n");
				return -1;
			}
			break;
		case 'T':
			timer_secs = fwd_parse_timer_period(optarg);
			if (timer_secs < 0) {
				printf("invalid timer period\n");
				return -1;
			}
			timer_period = timer_secs;
			break;
		case 0:
			break;
		default:
			return -1;
		}
	}
	if (optind >= 0)
		argv[optind-1] = prgname;
	ret = optind-1;
	optind = 1; /* reset getopt lib */
	return ret;
}
static void signal_handler(int signum)
{	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",signum);
		force_quit = true;
	}
}
int main(int argc, char **argv)
{	struct lcore_queue_conf *qconf;
	int ret;
	uint16_t nb_ports,nb_ports_available,portid;
	unsigned lcore_id, rx_lcore_id;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;
	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	ret = fwd_parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid fwd arguments\n");
	timer_period *= rte_get_timer_hz();
	fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF,
		MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
		rte_socket_id());
	if (fwd_pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
	nb_ports = rte_eth_dev_count();
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");	
	rx_lcore_id = 0;
	qconf = NULL;
	/* Initialize the port/queue configuration of each logical core */
	for (portid = 0; portid < nb_ports; portid++) {
		if ((fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		/* get the lcore_id for this port */
		while (rte_lcore_is_enabled(rx_lcore_id) == 0 ||
		       lcore_queue_conf[rx_lcore_id].n_rx_port ==
		       fwd_rx_queue_per_lcore) {
			rx_lcore_id++;
			if (rx_lcore_id >= RTE_MAX_LCORE)
				rte_exit(EXIT_FAILURE, "Not enough cores\n");
		}
		if (qconf != &lcore_queue_conf[rx_lcore_id])
			/* Assigned a new logical core in the loop above. */
			qconf = &lcore_queue_conf[rx_lcore_id];
		qconf->rx_port_list[qconf->n_rx_port] = portid;
		qconf->n_rx_port++;
		printf("Lcore %u: RX port %u\n", rx_lcore_id, portid);
	}
	nb_ports_available = nb_ports;
	/* Initialise each port */
	for (portid = 0; portid < nb_ports; portid++) {
		if ((fwd_enabled_port_mask & (1 << portid)) == 0) {
			printf("Skipping disabled port %u\n", portid);
			nb_ports_available--;
			continue;
		}
		/* init port */
		printf("Initializing port %u... ", portid);
		fflush(stdout);
		ret = rte_eth_dev_configure(portid, 1, 1, &port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",ret, portid);
	        rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd,  &nb_txd);
		rte_eth_macaddr_get(portid,&fwd_ports_eth_addr[portid]);
		/* init one RX queue */
		fflush(stdout);
	        rte_eth_rx_queue_setup(portid, 0, nb_rxd, rte_eth_dev_socket_id(portid),
					     NULL,  fwd_pktmbuf_pool);  	
		 fflush(stdout);
		 rte_eth_tx_queue_setup(portid, 0, nb_txd,
				rte_eth_dev_socket_id(portid),NULL);		
		rte_eth_dev_start(portid);
	//rte_eth_promiscuous_enable(portid);
		printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",portid,
				fwd_ports_eth_addr[portid].addr_bytes[0],
				fwd_ports_eth_addr[portid].addr_bytes[1],
				fwd_ports_eth_addr[portid].addr_bytes[2],
				fwd_ports_eth_addr[portid].addr_bytes[3],
				fwd_ports_eth_addr[portid].addr_bytes[4],
				fwd_ports_eth_addr[portid].addr_bytes[5]);
		memset(&port_statistics, 0, sizeof(port_statistics));
	}
	ret = 0;
	rte_eal_mp_remote_launch(fwd_launch_one_lcore, NULL, CALL_MASTER);
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			ret = -1;
			break;
		}
	}
	for (portid = 0; portid < nb_ports; portid++) {
		if ((fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}
	return ret;
}