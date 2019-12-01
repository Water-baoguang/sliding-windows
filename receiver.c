#include <stdlib.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <limits.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "header.h"
#include "boolean.h"
#include "checksum.c"
#include "queue.c"

//======DEKLARASI VARIABEL GROBAL=======================================
#define DELAY 500		/* Delay untuk menyesuaikan kecepatan konsumsi buffer (milidetik)*/
#define RXQSIZE 8		/* Ukuran buffer */
#define MIN_UPLIMIT 3	/* Minimum upper limit dari buffer*/
#define MAX_LOWERLIMIT 1/* Maximum lower limit dari buffer*/

MESGB rxbuf[RXQSIZE]; 	/* Data karakter dalam buffer */
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf }; /* Queue sebagau buffer */
QTYPE *rxq = &rcvq; 	/* Pointer ke queue buffer*/
Queue qRcv;				/* Container data yang sudah di ACK */

Byte sent_xonxoff = XON;/* Status XON/XOFF yang dikirim ke transmitter*/
boolean send_xon = true , send_xoff = false ;

Byte q; 				/* Karakter/data yang paling baru dikonsumsi dari buffer*/
int countByte =0;		/* Jumlah total data yang sudah dibaca dari transmitter*/
int consumedByte =0;	/* Jumlah total data yang sudah dikonsumsi dari buffer*/
Byte waitForNoMsg=0;	/* Nomor pesan yang di-NAK dan sedang ditunggu dapat di-ACK. 0=tidak ada yg belum di ACK*/
MESGB reRcv;			/* Kontainer MESGB yang diterima ulang*/
Queue qo;
int lastRcv=0,lastACK=0;
/* Socket */
int sockfd; /* ID socket*/
struct sockaddr_in rcvAdr; //Socket receiver
struct sockaddr_in sendAddr;//Socket transmitter
int sendAddrLen = sizeof(struct sockaddr *);
int rcvAddrLen;


/* Functions declaration */
static char * getIP(); 							//Fungsi untuk mengembalikan alamat IP dari receiver
static Byte *rcvchar( int sockfd, QTYPE *queue, MESGB *reRcv, int *lastRcv);//Fungsi untuk membaca data dari transmitter
static MESGB *q_get(QTYPE *queue, MESGB *data, MESGB reRcv, Byte * waitForNoMsg); 	//Fungsi untuk mengonsumsi data dari buffer
void * consProc(void * childid); 				//Child process berupa thread yang memanggil fungsi q_get
int checkFormat(MESGB rcvMes);

//========PROGRAM UTAMA=================================================
int main( int argc, char *argv[])
{
	CreateQueue(&qo);
	Byte c;//Karakter/data paling baru yang dibaca
	MESGB rcv;
	boolean succPort=-1;
	pthread_t childProc;
	CreateQueue(&qRcv);

	//Membuat socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd<0 || argc!=2) {
		printf("CANNOT CREATE SOCKET\n");
		return 0;
	}

	//Pengaturan socket
	rcvAddrLen = sizeof(rcvAdr);
	bzero(&rcvAdr,rcvAddrLen);
	memset((char *)&rcvAdr,0 , sizeof(rcvAdr));
	rcvAdr.sin_family = AF_INET;
	rcvAdr.sin_port = htons(atoi(argv[1]));
	rcvAdr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Bind socket ke port pada localhost receiver
	succPort = bind(sockfd, (struct sockaddr *)&rcvAdr, rcvAddrLen);
	if (succPort>=0) printf("Binding pada %s:%d ...\n", getIP(),htons(rcvAdr.sin_port));
	else {
		printf("Binding failed\n");
		return 0;
	}

	//Membuat child proses berupa thread untuk q_get
	pthread_create(&childProc, NULL, consProc, NULL);

	//Proses induk untuk memanggil rcvchar
	while (1){
		c = (rcvchar(sockfd,rxq,&reRcv, &lastRcv));
		if (c==Endfile) printf("END");
		usleep(DELAY*1000);

	}
	return 0;
}


//=========IMPLEMENTASI FUNGSI==========================================
//Fungsi yang akan mengembalikan alamat IP dari receiver
static char * getIP(){
    struct ifaddrs *ifaddr, *temp;
    char * ret = "127.0.0.1";

    int i = getifaddrs(&ifaddr);
    if(i == -1) return ret;
    temp = ifaddr;

	while(temp)
    {
		if(temp->ifa_addr && temp->ifa_name)
        {
			if(temp->ifa_addr->sa_family == AF_INET &&  strcmp(temp->ifa_name, "enp0s3")==0)
            {
                ret = inet_ntoa(((struct sockaddr_in*) temp->ifa_addr)->sin_addr);
                break;
            }
        }
        temp = temp->ifa_next;
    }
    freeifaddrs(ifaddr);
	return ret;
}

//Child proses berupa thread untuk memanggil q_get
void * consProc(void * childid){
	MESGB current;
	while(1){
		if (q_get(rxq,&current,reRcv, &waitForNoMsg)!= NULL){
			if (current.data>=32) printf("Memproses byte ke-%d:'%c'\n", current.msgno, current.data);
			else if (current.data==CR) printf("Memproses byte ke-%d: CR\n", current.msgno);
			else if (current.data==LF) printf("Memproses byte ke-%d: LF\n", current.msgno);
			else if (current.data==Endfile){
				MESRESP mesRet;
				if (sent_xonxoff==XOFF){
					sent_xonxoff = XON;
					send_xon = true;
					send_xoff = false;
					mesRet.ack = XON;
					char z[3] = {mesRet.msgno, mesRet.ack,'\0'};
					mesRet.checksum = crc16(z,2);
					int numBytesSent = sendto(sockfd,&mesRet, sizeof(mesRet), 0, (struct sockaddr *) &sendAddr, sendAddrLen);
				}
				 printf("Memproses byte k-%d: Endfile\n\n<Penerimaan data selesai>\nPesan: ", current.msgno);
				 printQ(qo);
				 exit(0);
			 }
			else printf("Memproses byte ke-%d: ASCII %d\n", current.msgno, current.data);
			printf("\n");
		}
		usleep(DELAY*5000);
	}
}

//Fungsi untuk menerima data dari transmitter dan memberikan keluaran berupa pointer data tersebut
static Byte *rcvchar( int sockfd, QTYPE *queue, MESGB *reRcv, int *lastRcv)
{
	Byte cTemp=NULL; //Penyimpanan sementara karakter yg dibaca
	MESGB mesT;
	if (sent_xonxoff==XON || waitForNoMsg!=0){
		ssize_t sbsize = recvfrom(sockfd, &mesT, sizeof(mesT), 0 , (struct sockaddr *)&sendAddr, &sendAddrLen);
		if (sbsize >=0){
			cTemp = mesT.data;

			//Bila data valid, maka data dimasukkan ke buffer
			if (cTemp>=32 || cTemp==LF || cTemp==CR || cTemp==Endfile){
				if (((waitForNoMsg==0)||(waitForNoMsg==0 && waitForNoMsg!=mesT.msgno))  && checkExist(&qRcv,mesT)==0){
					if (mesT.msgno > *lastRcv){//Pertama kali dikirim
						queue->data[queue->rear]=mesT;
						if (queue->rear == RXQSIZE-1){
							queue->rear = 0;
						}else{
							(queue->rear)++;
						}
						(queue->count)++;
						countByte++;
						printf("Menerima byte ke-%d %c.\n", mesT.msgno, cTemp);
						*lastRcv=mesT.msgno;
					}else printf("Menerima byte ke-%d %c.\n", mesT.msgno, mesT.data);//Yang timeout
				}
				else {
					//Yang di-NAK
					if (waitForNoMsg==mesT.msgno) (*reRcv) = mesT;
					printf("Menerima kembali byte ke-%d %c.\n", mesT.msgno, mesT.data);
				}
			}
		} else printf("Gagal menerima data\n");

	}
			//Bila isi buffer > minimum upper limit, maka mengirimkan XOFF
	if (queue->count > MIN_UPLIMIT && sent_xonxoff==XON){
		sent_xonxoff = XOFF;
		send_xon = false;
		send_xoff = true;
		MESRESP mesRet;
		mesRet.msgno = 0;
		mesRet.ack = XOFF;
		char z[3] = {mesRet.msgno, mesRet.ack,'\0'};
		mesRet.checksum = crc16(z,2);
		int sendFlowStat = sendto(sockfd,&mesRet, sizeof(mesRet), 0, (struct sockaddr *) &sendAddr, sendAddrLen);
		if (sendFlowStat >= 0 ) {
			printf("==Buffer > minimum upperlimit.\nMengirim XOFF\n");
		}
	}
	return &cTemp;
}

/* Fungsi untuk mengonsumsi data dalam buffer. Mengembalikan pointer menuju
 * bagian buffer yang menyimpan data atau NULL bila buffer kosong*/
static MESGB *q_get(QTYPE *queue, MESGB *data, MESGB reRcv, Byte* waitForNoMsg)
{
	/* Buffer kosong */
	int go=1;
	MESRESP mesRet;
	MESGB mesR;

		//Mengambil isi buffer
		if ((*waitForNoMsg)==0){
			if (queue->count > 0) {
				(mesR) = queue->data[queue->front];
				queue->count--;
				if (queue->front < (RXQSIZE-1)) {
					queue->front++;
				} else {
					queue->front = 0;
				}
				(*data) = mesR;
			}else return NULL;

			//Validasi data
			while (((*data).data <= 32) && ((*data).data != LF) && ((*data).data != CR) && ((*data).data != Endfile) && (queue->count > 0)) {
				if (queue->count > 0) {
					(mesR) = queue->data[queue->front];
					queue->count--;
					if (queue->front < (RXQSIZE-1)) {
						queue->front++;
					} else {
						queue->front = 0;
					}
				}
			}
		}else{
			if (reRcv.msgno == (*waitForNoMsg)) (*data)=reRcv;
			else {
				go =0;
				data=NULL;
			}
		}


		//Acknowlegdement
		//Pertimbangan = ACK/NAK & format frame
		if (go==1 && checkNumExist(&qRcv, (*data).msgno)==0){
			char xz[3] = {(*data).msgno, (*data).data,'\0'};
			unsigned short p = crc16(xz,2);

			//MESGB m = qRcv.Last->data;
			if ((*data).checksum == p && ((*data).soh==SOH && (*data).stx==STX && (*data).etx==ETX)){
				if (mesR.msgno> lastACK) lastACK = mesR.msgno;
				printf("\n>> ACK-ed %d. Siap menerima %d\n", mesR.msgno, lastACK+1);//mesR.msgno+4);
				mesRet.msgno = lastACK+1;//mesR.msgno+4;
				mesRet.ack = ACK;
				char z[3] = {mesRet.msgno, mesRet.ack,'\0'};
				mesRet.checksum = crc16(z,2);
				addRep(&qRcv, mesR); //MASUKIN KE QUEUE ACKed dengan urutan membesar
				if (mesR.data != Endfile) addRep(&qo, mesR);

				//if (waitForNoMsg==0) usleep(DELAY * (rand()%9)*1000);
				int sendFlowStat = sendto(sockfd, &mesRet, sizeof(mesRet), 0, (struct sockaddr *) &sendAddr, sendAddrLen);
				if ((*waitForNoMsg)==mesR.msgno) (*waitForNoMsg) =0;

			}else {
				printf("\n>> NAK-ed %d: menunggu pengiriman ulang\n", (*data).msgno);
				mesRet.msgno = (*data).msgno;
				mesRet.ack = NAK;
				char z[3] = {mesRet.msgno, mesRet.ack,'\0'};
				mesRet.checksum = crc16(z,2);
				int sendFlowStat = sendto(sockfd,&mesRet, sizeof(mesRet), 0, (struct sockaddr *) &sendAddr, sendAddrLen);
				(*waitForNoMsg) = (*data).msgno;
				data = NULL;
			}
		}

	//Bila jumlah data dalam buffer (count) kurang dari max_lowerlimit, maka mengirimkan XON
		if ((queue->count < MAX_LOWERLIMIT) && sent_xonxoff==XOFF){
			sent_xonxoff = XON;
			send_xon = true;
			send_xoff = false;
			mesRet.ack = XON;
			char z[3] = {mesRet.msgno, mesRet.ack,'\0'};
			mesRet.checksum = crc16(z,2);
			int numBytesSent = sendto(sockfd,&mesRet, sizeof(mesRet), 0, (struct sockaddr *) &sendAddr, sendAddrLen);
			if (numBytesSent >= 0) printf("==Buffer < maksimum lowerlimit.\nMengirim XON\n");
		}
		return data;
	}

void water(void);
