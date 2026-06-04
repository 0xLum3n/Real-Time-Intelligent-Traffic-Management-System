#include <stdio.h>
void sort(int arr[], int n)
{
 int i, j, temp;
 for(i = 0; i < n - 1; i++)
 {
 for(j = 0; j < n - i - 1; j++)
 {
 if(arr[j] > arr[j + 1])
 {
 temp = arr[j];
 arr[j] = arr[j + 1];
 arr[j + 1] = temp;
 }
 }
 }
}
int findPlatform(int arr[], int dep[], int n)
{
 sort(arr, n);
 sort(dep, n);
 int plat_needed = 1;
 int result = 1;
 int i = 1, j = 0;
 while(i < n && j < n)
 {
 if(arr[i] <= dep[j])
 {
 plat_needed++;
 i++;
 }
 else
 {
 plat_needed--;
 j++;
 }
 if(plat_needed > result)
 result = plat_needed;
 }
 return result;
}
int main()
{
 int arr[] = {900, 910, 940};
 int dep[] = {930, 1000, 1030};
 int n = 3;
 printf("Minimum Platforms Required = %d\n",
 findPlatform(arr, dep, n));
 return 0;
}
