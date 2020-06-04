
#include<iostream>
using namespace std;

//定义结构体
struct Node
{
	int data;
	struct Node *next; //指向自己数据类型的指针
};
//定义别名，Node和LinkList等价
typedef struct Node *LinkList;


//声明函数
int listInsert(LinkList L,int i,Node *node);

int add(LinkList L);
void addHead(LinkList L,LinkList node);

Node* delNode(LinkList L,int i);
int main()
{
	

//声明链表变量，链表就用指向节点的一个指针表示
    //一个指针，而不是多个。
    //也叫头指针。头指针就表示链表，
    LinkList L,p;

	//定义头节点变量
    Node n;
    n.data=0;
    n.next=NULL;

    //头指针指向头结点
    L=&n;

    int i=1;
    int length=9;
    //往头节点后的i加节点
    while(i <= length) {
    	listInsert(L,i,NULL);
    	i++;
    }

    //输出打印链表
    p=L;
    while (p != NULL) {
    	printf("结点值：%d 地址：%p \n",p->data,p);
    	p = p->next;
    }

    i=1;
    while(i <= length/2) {//length/2默认是向下取整
    	//单链表L1从头开始删除
    	p = delNode(L,length);
    	printf("L1删除值：%d 指针 %p \n",p->data,p);
    	
    	listInsert(L,2*i,p);
    	i++;
    }
    printf("单链表L1删除后\n");
    p=L;
    while (p != NULL) {
    	printf("结点值：%d 地址 %p \n",p->data,p);
    	p = p->next;
    }
    // printf("新的单链表L2\n");
    // p=L2;
    // while (p != NULL) {
    // 	printf("结点值：%d 地址 %p \n",p->data,p);
    // 	p = p->next;
    // }
    return 0;
}

/**
* 定义函数,插入指定位置之后，i=1，插在头结点之后
* 如果第三个参数存在，那么把它插入到i位置
*/
int listInsert(LinkList L,int i,Node *node =NULL) {
	int j=1;
	LinkList p,s;
	p = L;//p初始化为头指针，不是头结点，这个概念要区分开
	//p是头指针了，所以它指向头结点，*p是头结点，p是指向头结点的指针
	while(p != NULL && j < i) {
		//下一个节点的地址给p指针，因为p一开始指向头结点
		//所以下一个结点，就是第一个元素结点
		p = p->next;
		j++;
	}
	if (p == NULL || j >i) {
		//一般进入这里就是p走到尽头了，或者j=i；j > i只有一种情况，那就是i本身就比j小，也就是i比1小时；
		printf("%s \n","err");
		//printf("%d \n",p);
		return 1;
	}

	if (node == NULL) {
		s = (LinkList)malloc(sizeof(Node));
		s->data=i;	
	} else {
		s=node;
	}
	s->next=p->next;
	(*p).next=s;
	//p->next=s;
}

/**
* 	给单链表添加结点
*	头插法.
*。 是倒序单链表的一个基本操作步骤
*/
void addHead(LinkList L,LinkList node) {
	//添加的结点，写到函数里，不能用变量，因为参数变量的地址是固定的，后续的&node将会出问题
	// void addHead(LinkList L,Node node) {
	LinkList top,first;
	top=L;

	//头结点的下一个结点
	first = top->next;

	//新结点指向first
	node->next=first;
	//头结点指向新结点
	top->next=node;
}

/**
*按照位置，从单链表种删除该结点，
并返回结点
*/

Node* delNode(LinkList L,int i) {
	LinkList p,retNode;
	//头结点
	p=L;
	//printf("del删除结点的指针：%p \n",p);

	int j=1;
	//删除第i个元素，需要找到它前一个位置
	while(p->next != NULL && j < i ) {
		p = p->next;
		j++;
	}
	if (p->next == NULL || j > i) {
		printf("删除结点不存在");
		return NULL;
	}
	retNode = p->next;
	p->next=retNode->next;
	retNode->next=NULL;
	return retNode;
}

