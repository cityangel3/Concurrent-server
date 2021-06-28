/*
 * echoserveri.c - An iterative echo server
 */
/* $begin echoserverimain */
#include "csapp.h"
#define ORDER_PER_CLIENT 10
#define STOCK_NUM 10
#define BUY_SELL_MAX 10
#define PRICE_NUM 10000
//#define STOCK_MAX 20
typedef struct node{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex,w;
    struct node *left;
    struct node *right;
}Node;
Node* StockDB;
typedef struct stack{
    int id,left,price;
}Stack;

Node* addNode(Node** root,Node* item);
int getNode(Node* ptr,int Id,int num,int sb);
void freeNode(Node* ptr);
void printTree(Node* root);

int parseline(char *buf, char **tok);
void Echo(int connfd);
void readfile(void);
void updatefile(void);
int getdata(Node* root,Stack* data[]);

//#define MAX_CLIENT 100
void *thread(void *vargp);

sem_t mut;
int main(int argc, char **argv)
{
    
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;
    
    if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
    }
    
    readfile(); //stock.txt로부터 tree에 정보 저장.
    sem_init(&mut,0,1);
    listenfd = Open_listenfd(argv[1]); //받을 준비가 된 파일 디스크립터 생성
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = (int*)malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen); //서버 소켓에 클라언트 연결
        //연결됐다고 출력
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        //thread create
        Pthread_create(&tid,NULL,thread,connfdp);
    }
}
void *thread(void *vargp){
    int connfd = *((int *)vargp); // Connfd : connected descriptor
    Pthread_detach(pthread_self()); //매 client 마다 thread 생성 후 detach를 통해 thread를 독립적으로 시행 후 reaping
    free(vargp);
    Echo(connfd);   //execute for the connfd
    Close(connfd);
    updatefile();
    return NULL;
}
int parseline(char *buf, char **tok){
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
    buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
    tok[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
    while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    tok[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
        return 1;
    return 0;
}
/* $end echoserverimain */
int getNode(Node* ptr, int Id,int num,int sb){
    static int flag;
    if(ptr == StockDB) flag = 0;
    if(ptr!= NULL){
        if(ptr->ID == Id){
            if(sb == 0){ //sell
                flag = 0;
                P(&(ptr->mutex));
                ptr->left_stock = ptr->left_stock + num;
                V(&(ptr->mutex));
            }
            else if(sb == 1){ //buy
                if(ptr->left_stock < num) {flag = 1; return flag;}
                else flag = 0;
                P(&(ptr->mutex));
                ptr->left_stock = ptr->left_stock - num;
                V(&(ptr->mutex));
            }
            return flag;
        }
        getNode(ptr->left,Id,num,sb);
        getNode(ptr->right,Id,num,sb);
    }
    return flag;
}
Node* addNode(Node** root,Node* item){
    if((*root) == NULL){
        (*root) = (Node*)malloc(sizeof(Node));
        (*root)->right = (*root)->left = NULL;
        (*root)->left_stock = item->left_stock;
        (*root)->ID = item->ID;
        (*root)->price = item->price;
        Sem_init(&((*root)->mutex),0,1); //initialize mutex = 1
        Sem_init(&((*root)->w),0,1);
        (*root)->readcnt = 0;
        return *root;
    }
    else{
        if(item->left_stock < (*root)->left_stock)
            (*root)->left = addNode(&((*root)->left),item);
        else
            (*root)->right = addNode(&((*root)->right),item);
    }
    return *root;
}
void readfile(){
    FILE* fp;
    char line[700];
    int sid,sprice,sleft;
    if((fp = fopen("stock.txt","r")) == NULL) {printf("Read File Error!\n"); exit(-1);} //if fopen error
    while(fgets(line,700,fp) != NULL){ // read line by line until EOF
        Node * nNode = (Node*)malloc(sizeof(Node));
        sscanf(line,"%d %d %d",&sid,&sleft,&sprice);
        nNode->left_stock = sleft; nNode->price = sprice; nNode->ID = sid;
        addNode(&StockDB,nNode);
    }
    fclose(fp);
}
void updatefile(){
    P(&mut);
    Stack* data[MAXLINE];
    FILE* fp;
    int sidx;
    sidx = getdata(StockDB,data);
    if((fp = fopen("stock.txt","w")) == NULL) {printf("Read File Error!\n"); exit(-1);} //if fopen error
    for(int i=0;i<sidx;i++){
        fprintf(fp,"%d %d %d\n",data[i]->id,data[i]->left,data[i]->price);
        free(data[i]);
        data[i] = NULL;
    }
    fclose(fp);
    V(&mut);
}
int getdata(Node* root,Stack* data[]){
    static int sidx;
    if(root != NULL){
        P(&(root->w));
        root->readcnt++;
        if(root->readcnt == 1) /*first in*/
           P(&(root->mutex));
        V(&(root->w));
        
        if(root == StockDB)
            sidx = 0;
        
        data[sidx] = (Stack*)malloc(sizeof(Stack));
        data[sidx]->id = root->ID;
        data[sidx]->left = root->left_stock;
        data[sidx++]->price = root->price;
 
        P(&(root->w));
        root->readcnt--;
        if(root->readcnt == 0) /*last out*/
            V(&(root->mutex));
        V(&(root->w));
        
        getdata(root->left,data);
        getdata(root->right,data);
    }
    return sidx;
}
void printTree(Node* root){
    if(root == NULL)
        return;
    printf("%d  %d  %d\n",root->ID,root->left_stock,root->price);
    printTree(root->left);
    printTree(root->right);
}
int store(char* buffer,int connfd){ //cubf에 출력할 내용 저장.
    char cmdline[MAXLINE],*token[MAXLINE];
    int i,index = 0,sidx,flag;
    strcpy(cmdline,buffer);
    parseline(cmdline,token);
    memset(buffer,'\0',MAXLINE); //buffer 초기화
    /*for(int i =0;token[i] != NULL;i++){
        printf("%s\n",token[i]);
    }*/
    if(!(strcmp(token[0],"show"))){ //read
        P(&mut);
        Stack* data[MAXLINE];
        sidx = getdata(StockDB,data); //Root에서부터 접근
       // printf("*%d*",sidx);
        for(i =0;i<sidx;i++){
            sprintf(buffer+index,"%d %d %d",data[i]->id,data[i]->left,data[i]->price);
            index = strlen(buffer);
            buffer[index++] = '!';
            free(data[i]);
            data[i] = NULL;
        }
        buffer[index-1] = '\n';
        V(&mut);
    }
    else if(!(strcmp(token[0],"buy"))){ //write
        flag = getNode(StockDB,atoi(token[1]),atoi(token[2]),1); //Root에서부터 접근
        if(flag == 1)
            sprintf(buffer,"Not enough left stock\n");
        else
            sprintf(buffer,"[buy] success\n");
        index = strlen(buffer);
    }
    else if(!(strcmp(token[0],"sell"))){ //write
        flag = getNode(StockDB,atoi(token[1]),atoi(token[2]),0); //Root에서부터 접근
        sprintf(buffer,"[sell] success\n");
        index = strlen(buffer);
    }
    else if(!(strcmp(token[0],"exit"))){
        sprintf(buffer,"exit\n");
        index = strlen(buffer);
    }
    //printf("%s\n",buffer);
    return index;
}
