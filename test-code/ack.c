//#include<iostream>
//#include<time.h>
//using namespace std;
#include"myhead.h"
int loop;
int start;
int end;
char str1[10] = "result:\n";
char str2[10] = "time:";
char str3[10] = "seconds\n";
char str4[10] = "loops:";
int a;

int ack(int m,int n)
{
	if(m == 0)
		return n+1;
	else if(n == 0)
		return ack(m-1,1);
	else
		return ack(m-1,ack(m,n-1));
}

int main()
{
	print_s(str4);
	scan_d(&loop);
	time(&start);
	for(int i = 0; i < loop; i++){
		a = ack(3, 3);
	}
	time(&end);
	print_s(str1);
	print_d(a);
	print_c('\n');
	print_s(str2);
	print_d(end - start);
	print_s(str3);
    return 0;
}