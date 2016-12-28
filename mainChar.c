#include <stdio.h>
#include "queueChar.c"

int main(){
	int i=0;
	char c;
	QueueChar qC;
	
	CreateQueueC(&qC);
	while ((c=getchar())!='\n'){
		i++;
		addC(&qC,c,i);
	}
	printQC(qC);
	return 0;

}
