#include "header.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct QContentC *AddressC;

typedef struct QContentC {
  char data;
  int nomor;
  AddressC Next;
  AddressC Prev;
}QContentC;

typedef struct QueueChar{
	AddressC First;
	AddressC Last;
}QueueChar;

void CreateQueueC(QueueChar *Q){
	Q->First=0;
	Q->Last = 0;
}

int isEmptyC(QueueChar Q){
	return (Q.First==0);
}

void addC(QueueChar *Q, char data, int num){
	AddressC qC = (AddressC) malloc (sizeof(QContentC));
	qC->Next = 0;
	qC->Prev = 0;
	qC->data = data;
	qC->nomor = num;
	if (isEmptyC(*Q)==1){//kosong
		Q->First = qC;
		Q->Last = qC;
	}else{
		(Q->Last)->Next = qC;
		qC->Prev = Q->Last;
		Q->Last = qC;
	}
}

void deleteC(QueueChar *Q, int num){
	AddressC n=Q->First, prev=0;

	while (n!=0 && n->nomor!=num){
		prev = n;
		n = n->Next;
	}

	if (n!=0){//ada
		if (prev==0){//elemen pertama
			if (n->Next!=0){
				(n->Next)->Prev = 0;
				Q->First = n->Next;
			}else{
				Q->First = 0;
				Q->Last = 0;
			}
		}else{//bukan elemen pertama
			if (n->Next!=0){//bukan terakhir
				(n->Prev)->Next = n->Next;
				(n->Next)->Prev = n->Prev;
			}else{
				(n->Prev)->Next = 0;
				Q->Last = n->Prev;
			}

		}
		n->Prev = 0;
		n->Next = 0;
		free(n);
	}
}

AddressC getEl(QueueChar Q, int msgno){
	AddressC n = Q.First;
	while (n!=0 && n->nomor!=msgno){
		n = n->Next;
	}
	return n;
}

void printQC(QueueChar Q){
	AddressC n=Q.First;

	while (n!=0){
		printf("%c", n->data);
		n = n->Next;
	}
    printf("\n");
}

