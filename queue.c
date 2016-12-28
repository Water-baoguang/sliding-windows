#include "header.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct QContent *Address;

typedef struct QContent {
  MESGB data;
  Address Next;
  Address Prev;
}QContent;

typedef struct Queue{
	Address First;
	Address Last;
}Queue;

void CreateQueue(Queue *Q){
	Q->First=0;
	Q->Last = 0;
}

int isEmpty(Queue Q){
	return (Q.First==0);
}

void add(Queue *Q, MESGB data){
	Address qC = (Address) malloc (sizeof(QContent));
	qC->Next = 0;
	qC->Prev = 0;
	qC->data = data;
	if (isEmpty(*Q)==1){
		Q->First = qC;
		Q->Last = qC;
	}else{
		(Q->Last)->Next = qC;
		qC->Prev = Q->Last;
		Q->Last = qC;
	}
}

int addR(Queue *Q, MESGB data){
	Address qC = (Address) malloc (sizeof(Address));
	qC->Next = 0;
	qC->Prev = 0;
	qC->data = data;
	if (isEmpty(*Q)==1){
		Q->First = qC;
		Q->Last = qC;
	}else{
		(Q->Last)->Next = qC;
		qC->Prev = Q->Last;
		Q->Last = qC;
	}
	if (Q->Last==qC) return 1; else 0;
}

void addSorted(Queue *Q, MESGB data){
	if (isEmpty(*Q)==1) add(Q,data);
	else{
		Address n=Q->First;
		int stop=0;

		while (n!=0 && stop==0){
			if (data.msgno < (n->data).msgno) stop=1;
			else n=n->Next;
		}

		if (n!=0){//bukan first
			Address qC = (Address) malloc (sizeof(QContent));
			qC->data = data;
			if (n!=Q->First){
				qC->Prev = n->Prev;
				(n->Prev)->Next = qC;
				qC->Next = n;
				n->Prev = qC;
			}else{
				qC->Next = Q->First;
				n->Prev = qC;
				Q->First = qC;
				qC->Prev = 0;
			}

		}else{
			add(Q,data);
		}
	}
}

void delete(Queue *Q, int data){
	Address n=Q->First, prev=0;

	while (n!=0 && n->data.msgno!=data){
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

void printQ(Queue Q){
	Address n=Q.First;
	int stop=0;

	while (n!=0 && stop==0){
		printf("%c", (n->data).data);
		n = n->Next;
	}
  printf("\n");
}

int checkExist(Queue *Q, MESGB data) {
  Address n=Q->First, prev=0;

	while (n!=0 && n->data.msgno!=data.msgno){
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

int checkNumExist(Queue *Q, Byte msgno) {
  Address n=Q->First, prev=0;

	while (n!=0 && n->data.msgno!=msgno){
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

Address search(Queue *Q, int data) {
  Address n=Q->First, prev=0;

	while (n!=0 && n->data.msgno!=data){
		prev = n;
		n = n->Next;
	}
  return n;
}

void addRep(Queue *Q, MESGB data) {
  Address n = search(Q,data.msgno);
  if (n == 0) {
    addSorted(Q,data);
  }
  else {
    n->data = data;
  }
}

int Oke(Queue *Q, MESRESP data) {
  if (Q->First == 0) {
	return 1;
  }
  if (search(Q,data.msgno)->data.msgno == Q->First->data.msgno) {
    return 1;
  }
  else {
    return 0;
  }
}
/*
int main(){
	Queue Q;
	MESGB d;
	d.data = '1';
	CreateQueue(&Q);
	d.msgno=1;
	addSorted(&Q,d);
	d.msgno=2;
	addSorted(&Q,d);
	printf("%d", checkNumExist(&Q,1));
	printQ(Q);
	delete(&Q,1);
	printQ(Q);
	delete(&Q,2);
	d.msgno=2;
	printQ(Q);
	addSorted(&Q,d);
	delete(&Q,3);
	printQ(Q);
	delete(&Q,2);
	d.msgno=1;
	printQ(Q);
	addSorted(&Q,d);
printQ(Q);
	printf("%d", isEmpty(Q));
	return 0;
	}

*/
