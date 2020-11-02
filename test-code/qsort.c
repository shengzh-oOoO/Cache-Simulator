//#include<iostream>
//#include<time.h>
//using namespace std;
#include "myhead.h"
void exchange(int *a, int *b) {
    //print_d(*a);
    //print_d(*b);
    int temp = *a;
    *a = *b;
    *b = temp;
}
int partition(int *a, int p, int r) {
    int key = a[r];
    int i = p - 1;
    for (int j = p; j < r; j++)
    {  
        if (a[j] <= key)
        {          
            i++;       
            exchange(&a[i], &a[j]);
        }      
    }  
    exchange(&a[i + 1], &a[r]);
    return i + 1;
}
 
void quickSort(int *a, int p, int r) {
    int position = 0;
    if (p<r)
    {
        position = partition(a,p,r);
        quickSort(a,p,position-1);
        quickSort(a, position + 1,r);
    }  
}
int d[10] = {1,4,2,8,5,7,9,11,99, -1};
int loop;
int start;
int end;
char str1[10] = "result:\n";
char str2[10] = "time:";
char str3[10] = "seconds\n";
char str4[10] = "loops:";

int main() {
    print_s(str4);
    scan_d(&loop);
    time(&start);
    for(int i = 0; i < loop; i++){
        d[0] = 1;
        d[1] = 4;
        d[2] = 2;
        d[3] = 8;
        d[4] = 5;
        d[5] = 7;
        d[6] = 9;
        d[7] = 11;
        d[8] = 99;
        d[9] = -1;
        quickSort(d, 0, 9);
    }
    time(&end);
    print_s(str1);
    for(int i = 0; i < 10; i++){
        print_d(d[i]);
        print_c(' ');
    } 
    print_c('\n');
    print_s(str2);
    print_d(end-start);
    print_s(str3);
    return 0;
}
