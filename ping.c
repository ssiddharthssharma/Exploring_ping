
#include <stdio.h> 
#include <sys/types.h> 
#include <sys/time.h>
#include <sys/socket.h> 
 
#include <arpa/inet.h> 
#include <netdb.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h> 

#include <time.h> 
#include <fcntl.h> 
#include <signal.h> 
#include <errno.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <time.h> 


#define icmp_packet_send_interval 1
#define BUFFSIZ 1024
int main_loop = 1;
int send_packet_flag = 0;
int npkt_send = 0;



void handle_graceful_exit(int signum){
	main_loop = 0;
	printf("-------timeout------\n");
	printf("Stats:\n");
}

void send_icmp_packet_sig(int signum){
	send_packet_flag = 1;
}

void
tv_sub(struct timeval *out, struct timeval *in)
{
	if ( (out->tv_usec -= in->tv_usec) < 0) {	/* out -= in */
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

unsigned short
in_cksum(unsigned short *addr, int len)
{
	int				nleft = len;
	int				sum = 0;
	unsigned short	*w = addr;
	unsigned short	answer = 0;

	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

		if (nleft == 1) {
		*(unsigned char *)(&answer) = *(unsigned char *)w ;
		sum += answer;
	}
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

void create_and_send_icmp(int ping_sock,const char* host){

	char sendbuf[BUFFSIZ];
	struct icmp *icmp_pkt;
	npkt_send++;

	icmp_pkt = (struct icmp *) sendbuf;
	icmp_pkt->icmp_type = ICMP_ECHO;
	icmp_pkt->icmp_code = 0;
	icmp_pkt->icmp_id = getpid();
	//printf("%d\n", icmp_pkt->icmp_id );
	icmp_pkt->icmp_seq = npkt_send;
	//printf("%s\n",sendbuf );
	int datalen = 56;
	memset(icmp_pkt->icmp_data, 0xa5, datalen);
	gettimeofday((struct timeval *) icmp_pkt->icmp_data, NULL);
	size_t len = 64;
	icmp_pkt->icmp_cksum = in_cksum((u_short *) icmp_pkt, len);
	//printf("%d\n",icmp_pkt->icmp_cksum );

	struct hostent* hostaddr;

	hostaddr = gethostbyname(host);

	if(!hostaddr){
    printf("Could not resolve host name\n");
    exit(1);
	}


	struct sockaddr_in saddr;
	int addrlen = sizeof(saddr); 

	saddr.sin_family = hostaddr->h_addrtype; 
    saddr.sin_addr.s_addr  = *(long*)hostaddr->h_addr; 

	sendto(ping_sock, sendbuf, len, 0, (struct sockaddr *)&saddr, sizeof(saddr)  );

	char *address = inet_ntoa(*((struct in_addr*) hostaddr->h_addr_list[0])); 
	printf("icmp sequence:%d ping to %s(%s)", npkt_send, host, address);
  
}


void recieve_packet(int sockfd){

	struct ip *ip;
	struct icmp *icmp;
	struct timeval *sendtime;
	int icmplen;
	size_t len = 64;
	char ptr[BUFFSIZ];
	int lenn = sizeof(ptr);
	struct sockaddr_in r_addr;
	struct timeval *tvsend, *tvrecv;
	int addrlen = sizeof(r_addr);



	if(recvfrom(sockfd, ptr, lenn, 0, (struct sockaddr*)&r_addr, &addrlen) <= 0){
		printf("Packet recieved failed\n");
	}else{

			ip = (struct ip*) ptr;
			int hlen1 = ip->ip_hl << 2;
			if(ip->ip_p != IPPROTO_ICMP) return;
			icmp = (struct icmp *) (ptr + hlen1);
			if( (icmplen = len - hlen1) < 8 ) return;
			if(icmp-> icmp_type == ICMP_ECHOREPLY){
				if (icmp->icmp_id != getpid()) return;
				if (icmplen < 16) return;
				tvsend = (struct timeval *) icmp->icmp_data;
				gettimeofday(tvrecv, NULL);
				tv_sub(tvrecv, tvsend);
				double rtt = tvrecv->tv_sec*1000.0 + tvrecv->tv_usec /1000.0;
				printf("%d bytes from: seq=%u, ttl=%d, rtt=%.3f ms\n", icmplen, icmp->icmp_seq, ip->ip_ttl, rtt); 
			}
		}
	
}




int main(int argc, char const *argv[])
{	

	if(argc!=2) 
    { 
        printf(" Format %s <address>\n", argv[0]); 
        return 0; 
    } 

    printf("CMPE 207 HW5 ping siddharth sharma 334\n\n");


	int ping_sock;
	
	ping_sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ping_sock < 0){
		fprintf(stderr, "Cannot create socket\n");
	} else printf("Socket formed with return:%d\n", ping_sock );
	
	
	
	signal(SIGALRM, send_icmp_packet_sig);
	alarm(icmp_packet_send_interval);
	signal(SIGINT, handle_graceful_exit);

	printf("PING %s\n", argv[1]);

	while(main_loop){
		if(send_packet_flag == 1){
			create_and_send_icmp(ping_sock, argv[1]);
			printf("Sending ICMP packet\n");
			alarm(icmp_packet_send_interval);
			signal(SIGALRM, send_icmp_packet_sig);
			recieve_packet(ping_sock);

			send_packet_flag = 0;
		}
		
	}
	
	return 0;

}