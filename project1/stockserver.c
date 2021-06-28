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

typedef struct {
    int maxfd, nready, maxi, clientfd[MAXLINE];
    fd_set ready_set;
    fd_set read_set;
    rio_t clientrio[MAXLINE];
}pool;
int byte_cnt = 0;

Node* addNode(Node** root,Node* item);
int getNode(Node* ptr,int Id,int num,int sb);
void freeNode(Node* ptr);
void printTree(Node* root);

int parseline(char *buf, char **tok);
void Echo(int connfd);
void readfile(void);
void updatefile(void);
int getdata(Node* root,Stack* data[]);

void *thread(void *vargp);

void init_pool(int listenfd,pool *p);
void add_client(int connfd ,pool *p);
void check_clients(pool* p);

int main(int argc, char **argv)
{
    
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool pool;
    //fd_set read_set, ready_set; //for event-based
    
    if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
    }
    
    readfile(); //stock.txt로부터 tree에 정보 저장.
    
    listenfd = Open_listenfd(argv[1]); //받을 준비가 된 파일 디스크립터 생성
    init_pool(listenfd,&pool);
    

    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1,&pool.ready_set,NULL,NULL,NULL);
        
        if(FD_ISSET(listenfd,&pool.ready_set)){
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,client_port, MAXLINE, 0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
           
            add_client(connfd,&pool);
        }
        check_clients(&pool);
        updatefile();
    }
}
void init_pool(int listenfd,pool *p){
    /* Initially, there are no connected descriptors */
    int i;
    p->maxi = -1;
    for ( i=0; i< FD_SETSIZE; i++)
    p->clientfd[i] = -1;
    /* Initially, listenfd is only member of select read set */
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}
void add_client(int connfd ,pool *p){
    int i;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++) /* Find an available slot */
    if (p->clientfd[i] < 0) {
        p->clientfd[i] = connfd;
        Rio_readinitb(&p->clientrio[i], connfd); /*rio_t 구조체 초기화, 버퍼 사용-> rio_readlineb and rio_readnb*/

        FD_SET(connfd, &p->read_set); /*descriptor set에 추가*/
        /* Update max descriptor and pool high water mark */
        if (connfd > p->maxfd)
            p->maxfd = connfd;
        if (i > p->maxi)
            p->maxi = i;
        break;
    }
    if (i == FD_SETSIZE) /* Couldn’t find an empty slot */
        app_error("add_client error: Too many clients");
}
void check_clients(pool* p){
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;
    
    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];
        /* If the descriptor is ready, echo a text line from it */
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                printf("server received %d bytes\n", n);
                n = store(buf,connfd);
                Rio_writen(connfd, buf, n);
            }
            else { /* EOF detected, remove descriptor from pool */
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }
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
                ptr->left_stock = ptr->left_stock + num;
            }
            else if(sb == 1){ //buy
                if(ptr->left_stock < num) {flag = 1; return flag;}
                else flag = 0;
                ptr->left_stock = ptr->left_stock - num;
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
    Stack* data[MAXLINE];
    FILE* fp;
    int sidx;
    if((fp = fopen("stock.txt","w")) == NULL) {printf("Read File Error!\n"); exit(-1);} //if fopen error
    sidx = getdata(StockDB,data);
    for(int i=0;i<sidx;i++){
        fprintf(fp,"%d %d %d\n",data[i]->id,data[i]->left,data[i]->price);
        free(data[i]);
    }
    fclose(fp);
}
int getdata(Node* root,Stack* data[]){
    static int sidx;
    if(root == StockDB)
        sidx = 0;
    if(root != NULL){
        data[sidx] = (Stack*)malloc(sizeof(Stack));
        data[sidx]->id = root->ID;
        data[sidx]->left = root->left_stock;
        data[sidx++]->price = root->price;
        
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
void freeNode(Node* ptr){
    
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
        Stack* data[MAXLINE];
        sidx = getdata(StockDB,data); //Root에서부터 접근
        for(i =0;i<sidx;i++){
            sprintf(buffer+index,"%d %d %d",data[i]->id,data[i]->left,data[i]->price);
            index = strlen(buffer);
            buffer[index++] = '!';
            free(data[i]);
        }
        buffer[index-1] = '\n';
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

