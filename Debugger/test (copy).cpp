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
	cin>>a;
	cout<<"You just typed"<<endl<<a;

	callme();
	displayme();

	int count = 0;
	for(int i =0;i<20;i++){
		count++;
	}

	printf("count is %d",count);
	printf("End");
	return 0;
}
 0;
}
