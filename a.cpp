/**
输入一个数字，输出金字塔形状的数字层
比如3
        1
      1 3 1
    1 3 5 3 1
*/
#include<iostream>
using namespace std;
int main()
{
    int i, n,j,m,k;
    cout<<"请输入数字\n";
    cin>>n;
    //管层数，也就是高度
    for(i = 1; i <= n; i++)
    {
        for(j = 1; j<i; ++j)//左边数字
        { 
	       cout<<2*j-1;
	    }
	    cout<<2*i-1;//中间数字
        for (k=i-1;k>0;--k)//右边数字
        {
            cout<<2*k-1;
        }
        //换行
        cout<<endl;
    }
    return 0;
}