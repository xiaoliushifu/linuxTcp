
#include<iostream>
using namespace std;

char* func(char *p);
int main()
{
	#define str "hello world"
	char *p=NULL;
	
	p = (char *)malloc(strlen(str) + 1);
	strcpy(p, str); 
	printf("%s \n",p);
	p = func(p);
	printf("p2 %s \n",p);
	//func(p);
    return 0;
}


char* func(char *p) {

	char *tmp=NULL;
	tmp = (char *)malloc(sizeof(char));
	strcpy(tmp,"world hello");
	return tmp;

}

