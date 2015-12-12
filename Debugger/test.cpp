#include<iostream>
#include <stdio.h>

using namespace std;
int v = 35;
int newvar = 200;
char c = 'n';

void callme(){
	printf("Yeah, I called you\n");
}

void displayme(){
	printf("Yeah, I am displaying you\n");
}

int main(int argc, char *argv[]){

	char a;
	cout<<"Welcome To My Greatest Invention";
	cout<<"Value of a is "<<endl<<a;

	callme();
	displayme();

	for(int j=0;j<10;j++){
		cout<<"Hello suraj";
	}

	int count = 0;
	for(int i =0;i<5;i++){
		count+=2;
	}

	printf("count is %d",count);
	printf("End");
	return 0;
}
