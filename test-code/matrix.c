#include "myhead.h"
int loop;
int start;
int end;
char str1[10] = "result:\n";
char str2[10] = "time:";
char str3[10] = "seconds\n";
char str4[10] = "loops:";
int result[4][3] = { 0 };
int a[4][5] = { { 1, 3, 4, 5, 6 }, { 6, 3, 8, 9, 10 }, { 1, 2, 3, 4, 5 }, { 5, 6, 7, 8, 9 } };  
int b[5][3] = { { 1, 1, 1 }, { 2, 2, 2 }, { 3, 1, 4 }, { 3, 8, 8}, { 7, 6 ,3 } }; 
int main()
{
    print_s(str4);
    scan_d(&loop);
    time(&start);
    for(int l = 0; l < loop; l++){
        for (int i = 0; i < 4; i++) 
        {
            for (int j = 0; j < 3; j++) 
            {
                result[i][j] = 0;
                for (int k = 0; k < 5; k++) 
                {
                    result[i][j] += a[i][k] * b[k][j];
                }
            }
        }
    }
    time(&end);
    print_s(str1);
    for (int i = 0; i < 4; i++) 
    {
        for (int j = 0; j < 3; j++) 
        {
            print_d(result[i][j]);
            print_c('\t');
        }
        print_c('\n');
    }
    print_s(str2);
    print_d(end-start);
    print_s(str3);
    return 0;
}
