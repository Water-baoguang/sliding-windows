#include "header.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct QContentI *AddressI;

typedef struct QContentI {
  int data;
  AddressI Next;
  AddressI Prev;
}QContentI;

typedef struct QueueInt{
	AddressI First;
	AddressI Last;
}QueueInt;

void CreateQueueI(QueueInt *Q){
	Q->First=0;
	Q->Last = 0;
}

int isEmptyI(QueueInt Q){
	return (Q.First==0);
}

int checkExistInt(QueueInt *Q, int d) {
  AddressI n=Q->First, prev=0;

	while (n!=0 && (n->data) != d){
		prev = n;
		n = n->Next;
	}
  if (n != 0) {
    return 1;
  }
  else {
    return 0;
  }
}

void addI(QueueInt *Q, int data){
	AddressI qC = (AddressI) malloc (sizeof(QContentI));
	qC->Next = 0;
	qC->Prev = 0;
	qC->data = data;
	if (isEmptyI(*Q)==1){//kosong
		Q->First = qC;
		Q->Last = qC;
	}else{
		(Q->Last)->Next = qC;
		qC->Prev = Q->Last;
		Q->Last = qC;
	}
}

void deleteI(QueueInt *Q, int data){
	AddressI n=Q->First, prev=0;

	while (n!=0 && n->data!=data){
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

void printQI(QueueInt Q){
	AddressI n=Q.First;

	while (n!=0){
		printf("%d ", n->data);
		n = n->Next;
	}
    printf("\n");
}
