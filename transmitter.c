/* ANGGOTA KELOMPOK	: Sashi Novitasari		/13514027
 * 					  Kristianto Karim		/13514075
 * 					  Drestanto Dyasputro 	/13514099
 * PROGRAM			: Transmitter Simulasi Sliding Window Protocol
 * TANGGAL			: 1 Desember 2016
 * KETERANGAN		: Program dalam bahasa C yang menjalankan fungsi 
 * 					  transmitter pada simulasi sliding window protocol.
 * 					  Data dari transmitter dikirim dengan datagram */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "checksum.c"
#include "header.h"
#include "queue.c"
#include "queueInt.c"
#include "queueChar.c"
#define DELAY 500
#define WINSIZE 4

Byte receive_sig;			//Sinyal XON / XOFF yang diterima
struct sockaddr_in remAddress;
int sockfd, slen=sizeof(remAddress);//Socket
Queue qSent;				//Queue data MESGB yang udah dikirim
QueueChar pesan;			//Queue char yang mengandung char dari pesan (terurut)

int waitForNoMsg=0;			//Nomor char terkirim yang di-NAK
int lastsent=0;				//Nomor char terakhir yang dikirim
int rcved=0;				//Nomor char terakhir yang di-ACK
int size=0;					//Panjang pesan
int go=1;					//Status yang menandakan apakah sebagian dari thread boleh dijalankan/tidak (1=aktif, 0=non-aktif)

void error(char * msg) {
	printf(msg);
	exit(1);
}

void * recACK (void * threadACKReceiver) {
	MESRESP c;
	while(1) {		
		while (go==0) usleep(DELAY);
		int recvlen = recvfrom(sockfd, &c, sizeof(c), 0, (struct sockaddr *)&remAddress, &slen);
		if (recvlen > 0) {
			//Menerima ACK
			if (c.ack == ACK) {
				if (c.msgno >rcved){
					printf(">> ACK-ed %d\n", c.msgno-1);
					if (rcved<c.msgno-1) rcved = c.msgno-1;
					c.msgno -= 1;
					if (waitForNoMsg==c.msgno) waitForNoMsg=0;
					if (rcved>=size) {
						printf("\n<Transmisi data selesai>\n");
						close(sockfd);
						exit(0);
					}
				}
			//Menerima NAK
			}else if (c.ack == NAK){
				Address qC = search(&qSent,c.msgno);
				if (qC->data.msgno > rcved){
					printf(">> NAK-ed %d: ", c.msgno);
					waitForNoMsg = c.msgno;
					if (qC != 0) {
						(qC->data).soh = SOH;
						(qC->data).stx = STX;
						(qC->data).etx = ETX;
						if(sendto(sockfd, &qC->data, sizeof(qC->data), 0, (struct sockaddr *)&remAddress, slen) < 0) {
							error("SENDTO ERROR INI");
						}else{
							if (qC->data.data>=32) printf("Mengirim ulang byte ke-%d: '%c'\n",qC->data.msgno,qC->data.data);
							else if (qC->data.data==CR) printf("Mengirim ulang byte ke-%d: CR\n", qC->data.msgno);
							else if (qC->data.data==LF) printf("Mengirim ulang byte ke-%d: LF\n", qC->data.msgno);
							else printf("Mengirim ulang byte ke-%d: ASCII %d\n", qC->data.msgno,qC->data.data);
						}
					}
				}
			//Menerima XON/XOFF
			}else {
					receive_sig = c.ack;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	MESGB sentData;
	AddressC charQ=NULL;
	int pil, counter = 0, l, cur=0;
	Byte c = SOH, d;
	pthread_t ackRec;
	char h[3];
	
	CreateQueue(&qSent);
	CreateQueueC(&pesan);
	
	//=====INPUT PESAN===================================================================
	do{
		printf("Metode input pesan?\n1. Input Manual\n2. Input File\nPilihan: ");
		scanf("%d%c", &pil,&d);
		if (pil>2|| pil<1) printf("-- Masukan tidak valid --\n\n");
	}while((pil>2) || (pil<1));
	
	if (pil==1){//Input pesan dari terminal
		printf("\nPesan: ");
		while ((d=getchar())!='\n'){
			size++;
			addC(&pesan,d,size);
		}
		size++;
		addC(&pesan,Endfile,size);
	}else{//Input pesan dari file. Nama file dituliskan pada argv
		if (argc < 4) {
			error("Usage : ./transmitter <IP> <PORT> <FILE>");
		}
		char *file = argv[3];
		FILE *fp = fopen(file,"r");
		while (!feof(fp)) {
			d = fgetc(fp);
			size++;
			addC(&pesan,d,size);
		}
		addC(&pesan,26,size);
	}
	printf("\n");
	charQ = pesan.First;
	
	char *IP = argv[1];	int port = atoi(argv[2]);	
	if (!strncmp(IP,"localhost",9)) {
		bzero((char *)&IP,sizeof(IP));
		IP = "127.0.0.1";
	}
	printf(">> transmitter %s %d\n",IP,port);

	if ((sockfd=socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		error("SOCKET ERROR ");
	}
	else {
		printf("Membuat socket untuk koneksi ke %s:%d\n",IP,port);
	}

	bzero((char *) &remAddress, sizeof(remAddress));
	remAddress.sin_family = AF_INET;
	remAddress.sin_port = htons(port);

	if (inet_aton(IP, &remAddress.sin_addr)== 0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	sentData.soh = SOH;
	sentData.stx = STX;
	sentData.etx = ETX;
	sentData.msgno = 0; //GANTI
	sentData.data = SOH;
	h[0] = sentData.msgno;
	h[0] = sentData.data;
	h[0] = '\0';
	sentData.checksum = crc16(h,2);

	if(sendto(sockfd, &sentData, sizeof(sentData), 0, (struct sockaddr *)&remAddress, slen) < 0) {
		error("SENDTO ERROR");
	}
	
	//Child process untuk menangani XON XOFF ACK NAK
	pthread_create(&ackRec, NULL, recACK, NULL);
		
	while (rcved<size) {
		Address qC = qSent.First;
		cur = lastsent-rcved;
		
		//KIRIM NORMAL
		if (lastsent<size) {
			printf("\n");
			for (l = 0 ; l < WINSIZE-cur ; l++)		{
				go=0;
				if (counter+1<=size){
					if (counter+1!=size) charQ=getEl(pesan,counter+1);
					else{
						charQ->data=Endfile;
					}
					c = charQ->data;
					if (c < 255) {
						sentData.soh = SOH;
						sentData.stx = STX;
						sentData.etx = ETX;
						sentData.msgno = counter+1; //GANTI
						sentData.data = c;
						h[0] = sentData.msgno;
						h[1] = sentData.data;
						h[2] = '\0';
						sentData.checksum = crc16(h,2);
						addSorted(&qSent, sentData);
						counter++;
					}
				}
			}	
				
			if (lastsent==size) break;
			go=1;
			
			qC = qSent.First;
			while (qC->data.msgno <= lastsent) qC=qC->Next;
			
			//Pengiriman normal (!= retransmisi)
			while(qC != 0) {
				go=0;
				if (receive_sig == XOFF) {
					printf("---XOFF diterima.\n");
					while (receive_sig == XOFF) {
						printf("---Menunggu XON...\n");
						sleep(1);
					}
					printf("XON diterima.\n");
				}
				
				if(sendto(sockfd, &qC->data, sizeof(qC->data), 0, (struct sockaddr *)&remAddress, slen) < 0) {
					error("SENDTO ERROR");
				}else{
					if (qC->data.data>=32) printf("Mengirim byte ke-%d: '%c'\n",qC->data.msgno,qC->data.data);
					else if (qC->data.data==CR) printf("Mengirim byte ke-%d: CR\n", qC->data.msgno);
					else if (qC->data.data==LF) printf("Mengirim byte ke-%d: LF\n", qC->data.msgno);
					else printf("Mengirim byte ke-%d: ASCII %d\n", qC->data.msgno,qC->data.data);
					lastsent = qC->data.msgno;
					qC = qC->Next;
				}
				go=1;
				usleep(DELAY * 1000);	
			}
		}
		printf("\n");

		usleep(DELAY * 3000);
	
		//---------TIMEOUT----------------------------
		qC = qSent.First;
		while (qC->data.msgno <= rcved) qC=qC->Next;
		
		while(qC != 0 && rcved<size) {			
			if (isEmpty(qSent)==1) break;
			
			if (waitForNoMsg==0){	
				usleep(DELAY * 4000);
				if (receive_sig == XOFF) {
					printf("XOFF diterima.\n");
					while (receive_sig == XOFF) {
						printf("Menunggu XON...\n");
						sleep(1);
					}
					printf("XON diterima.\n");
				}
				go=0;
				qC=qSent.First;
				while (qC!=0 && qC->data.msgno <= rcved) qC=qC->Next;
				
				if (qC != 0) {
					if (waitForNoMsg != qC->data.msgno){ 
						(qC->data).soh = SOH;
						(qC->data).stx = STX;
						(qC->data).etx = ETX;
						if(sendto(sockfd, &qC->data, sizeof(qC->data), 0, (struct sockaddr *)&remAddress, slen) < 0) {
							error("SENDTO ERROR");
						}else{
							printf("Timeout %d: ", qC->data.msgno);
							if (qC->data.data>=32) printf("Mengirim ulang byte ke-%d: '%c'\n",qC->data.msgno,qC->data.data);
							else if (qC->data.data==CR) printf("Mengirim ulang byte ke-%d: CR\n", qC->data.msgno);
							else if (qC->data.data==LF) printf("Mengirim ulang byte ke-%d: LF\n", qC->data.msgno);
							else printf("Mengirim ulang byte ke-%d: ASCII %d\n", qC->data.msgno,qC->data.data);
							qC = qC->Next;
						}
					}
				}
				go=1;
			}
		}
	}
	return 0;
}
