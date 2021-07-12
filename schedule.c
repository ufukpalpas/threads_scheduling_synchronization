#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

struct node {
    int threadIndex;
    int burstIndex;
    int burstLength;
    long int creationTime;
    struct node* next;
};

struct params {
    int bcount;
    int avgb;
    int minb;
    int avga;
    int mina;
    int index;
};

struct sprm {
    char* str;
    int n;
    int* endAt;
    int isFileRd;
};

struct paramfile {
    FILE *fp;
    int k;
};

struct node* head = NULL;
struct node* cur = NULL;
pthread_mutex_t lock;
pthread_cond_t cv;
int cond;
int* thvruntime;
int runtimecount;
int threadfinished = 0;
long int* experimentArr;
int exparrcount;

void addToLinkedList(struct node* newNode){
    if(head == NULL){
        head = newNode;
        cur = head;
    } else {
        cur->next = newNode;
        cur = cur->next;
    }
    //printf("%d\n", cur->threadIndex);
}

void generateBurst(struct params* prm, int runUntil){
    double x;
    double r;
    double inter_arr;
    struct timeval time;
    double inter_arr_lambda = 1.0 / prm->avga;
    double burstlambda = 1.0 / prm->avgb;

    if(runUntil == 0)
        return;

    do{ //calculate burst length
        r = random() / (RAND_MAX + 1.0); // [0,1)
        x = (-1 * log(1 - r)) / burstlambda;
    } while(x < prm->minb);

    struct node* newNode = (struct node*) malloc(sizeof(struct node));
    newNode->threadIndex = prm->index;
    newNode->burstIndex = prm->bcount - runUntil + 1;
    newNode->burstLength = x;
    gettimeofday(&time, NULL);
    newNode->creationTime = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    newNode->next = NULL;

    pthread_mutex_lock(&lock);
    addToLinkedList(newNode);
    printf("\nNew node added => ThID: %d | BurstIndex: %d | BurstLength: %d | CreationTime : %ld\n", newNode->threadIndex, newNode->burstIndex, newNode->burstLength, newNode->creationTime);
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&lock);
    do{ //calculate inter-arrival time
        r = random() / (RAND_MAX + 1.0); // [0,1)
        inter_arr = (-1 * log(1 - r)) / inter_arr_lambda;
    } while(inter_arr < prm->mina);
    printf("\nThread %d will sleep %f ms\n", prm->index, inter_arr);
    usleep(inter_arr * 1000);
    generateBurst(prm, runUntil -1);
}

void *createBurst(void *args){
    struct params* prm = args;
    double r;
    double sleep;
    double lambda = 1.0 / prm->avga;
    do{
        r = random() / (RAND_MAX + 1.0); // [0,1)
        sleep = (-1 * log(1 - r)) / lambda;
    } while( sleep < prm->mina);
    printf("\nThread id: %d | sleep: %f\n", prm->index, sleep);
    usleep(sleep * 1000); //Sleep at the beginning of the thread
    int runUntil = prm->bcount;
    generateBurst(prm, runUntil);
    pthread_exit(0);
}

void fcfs(struct node** nodeptr){
    *nodeptr = head;
}

void sjf(struct node** nodeptr, int n){
    int oldTh[n];
    int burstLength = head->burstLength;
    struct node* ptr = head;
    int count = 0;
    *nodeptr = head;
    oldTh[0] = head->threadIndex;
    count++;
    while(ptr->next != NULL){
        int k = 1;
        for( int i = 0; i < count; i++){
            if((ptr->next)->threadIndex == oldTh[i]){
                k = 0;
                break;
            }
        }
        if(k == 1 && (ptr->next)->burstLength < burstLength){
            burstLength = (ptr->next)->burstLength;
            *nodeptr = ptr->next;
            oldTh[count] = (ptr->next)->threadIndex;
            count++;
        }
        ptr = ptr->next;
    }
}

void prio(struct node** nodeptr){
    int min_th_index = head->threadIndex;
    int min_burst_index = head->burstIndex;
    struct node* ptr = head;
    *nodeptr = head;
    while(ptr->next != NULL){
        if((ptr->next)->threadIndex < min_th_index){
            min_th_index = (ptr->next)->threadIndex;
            min_burst_index = (ptr->next)->burstIndex;
            *nodeptr = ptr->next;
        } else if((ptr->next)->threadIndex == min_th_index){
            if((ptr->next)->burstIndex < min_burst_index){
                min_burst_index = (ptr->next)->burstIndex;
                *nodeptr = ptr->next;
            }
        }
        ptr = ptr->next;
    }
}

void vruntime(struct node** nodeptr, int n){
    struct node* ptr = head;
    int tholds[n];
    int count = 0;
    *nodeptr = head;
    int min_vruntime = thvruntime[head->threadIndex - 1];
    tholds[0] = head->threadIndex;
    count++;
    while(ptr->next != NULL){
        if(thvruntime[(ptr->next)->threadIndex - 1] < min_vruntime){
            int k = 1;
            for( int i = 0; i < count; i++){
                if((ptr->next)->threadIndex == tholds[i]){
                    k = 0;
                    break;
                }
            }
            if(k == 1){
                min_vruntime = thvruntime[(ptr->next)->threadIndex - 1];
                *nodeptr = ptr->next;
                tholds[count] = (ptr->next)->threadIndex;
                count++;
            }
        }
        ptr = ptr->next;
    }
    thvruntime[(*nodeptr)->threadIndex - 1] += ((*nodeptr)->burstLength)*(0.7 + (0.3 * ((*nodeptr)->threadIndex)));
}

void *sthreadmanage(void *prm){
    pthread_mutex_lock(&lock);
    if(head == NULL){
        printf("\nQUEUE EMPTY (WAITING...)\n");
        pthread_cond_wait(&cv, &lock);
    }
    struct sprm* prms = prm;
    char* str = prms->str;
    int n = prms->n;
    struct node* execBurst;
    double length = 0;
    struct timeval time;
    
    if(strcmp(str, "FCFS") == 0){
        fcfs(&execBurst);
    } else if(strcmp(str, "SJF") == 0){
        sjf(&execBurst, n);
    } else if(strcmp(str, "PRIO") == 0){
        prio(&execBurst);
    } else if(strcmp(str, "VRUNTIME") == 0){
        vruntime(&execBurst, n);
    } 

    if(head == execBurst){
        //usleep((execBurst->burstLength));
        gettimeofday(&time, NULL);
        long int t = (time.tv_sec * 1000) + (time.tv_usec / 1000);
        long int w_time = t - execBurst->creationTime;
        printf("\nWait time of the thread whose details are below is %ld. [current time: %ld]", w_time, t);
        if(prms->isFileRd == 0)
            experimentArr[execBurst->threadIndex - 1] += w_time;
        length = execBurst->burstLength;
        head = head->next;
        printf("\n--DELETED (HEAD)=> ThID: %d | BurstIndex: %d | BurstLength: %d | CreationTime : %ld\n", execBurst->threadIndex, execBurst->burstIndex, execBurst->burstLength, execBurst->creationTime);
        free(execBurst);
        if(head == NULL)
            cur = NULL;
        (*prms->endAt)--;
    } else {
        struct node* prev = NULL;
        struct node* ptr = head;
        while(1){
            if(ptr != execBurst){
                prev = ptr;
                ptr = ptr->next;
            } else {
                prev->next = ptr->next;
                if(prev->next == NULL)
                    cur = prev;
                break;
            }
        }
        //usleep((execBurst->burstLength));
        gettimeofday(&time, NULL);
        long int t = (time.tv_sec * 1000) + (time.tv_usec / 1000);
        long int w_time = t - execBurst->creationTime;
        printf("\nWait time of the thread whose details are below is %ld. [current time: %ld]", w_time, t);
        if(prms->isFileRd == 0)
            experimentArr[execBurst->threadIndex - 1] += w_time;
        length = execBurst->burstLength;
        printf("\n--DELETED => ThID: %d | BurstIndex: %d | BurstLength: %d | CreationTime : %ld\n", execBurst->threadIndex, execBurst->burstIndex, execBurst->burstLength, execBurst->creationTime);
        free(execBurst);
        (*prms->endAt)--;
    }
    pthread_mutex_unlock(&lock);
    usleep(length * 1000);
    //printf("\n--- 1. %d  2. %d 3. %d | endAt: %d \n", thvruntime[0], thvruntime[1], thvruntime[2], (*prms->endAt));

    if(prms->isFileRd == 0){
        if((*prms->endAt) != 0)
            sthreadmanage(prm);
    } else {
        if(!(n == threadfinished && head == NULL))
            sthreadmanage(prm);
    }
    pthread_exit(0);
}

void initVruntime(int n){
    thvruntime = malloc(sizeof(int) * n);
    runtimecount = 0;
    for(int i = 0; i < n; i++)
        thvruntime[i] = 0;
}

void initexparr(int n){
    experimentArr = malloc(sizeof(long int) * n);
    exparrcount = 0;
    for(int i = 0; i < n; i++)
        experimentArr[i] = 0;
}

void genBurstFrFl(int interarr, int bursttime, int thindex, int burstindex){
    struct timeval time;
    struct node* newNode = (struct node*) malloc(sizeof(struct node));
    usleep(interarr * 1000);
    newNode->threadIndex = thindex;
    newNode->burstIndex = burstindex;
    newNode->burstLength = bursttime;
    gettimeofday(&time, NULL);
    newNode->creationTime = (time.tv_sec * 1000) + (time.tv_usec / 1000);
    newNode->next = NULL;
    
    pthread_mutex_lock(&lock);
    addToLinkedList(newNode);
    printf("\nNew node added => ThID: %d | BurstIndex: %d | BurstLength: %d | CreationTime : %ld\n", newNode->threadIndex, newNode->burstIndex, newNode->burstLength, newNode->creationTime);
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&lock);
}

void increaseThCo(){
    threadfinished++;
}

void *burstsFl(void* args){
    struct paramfile* prm = args;
    char getline[255]; 
    int burst_count = 1; 
    while(fgets(getline, sizeof(getline), (prm->fp))){
        char* split;
        char* splited[2];
        char* str = getline;
        int intarr, bursttime;
        for(int m = 0; m < 2; m++){
            split = strsep(&str, " ");
            splited[m] = split;
        }
        intarr = atoi(splited[0]);
        bursttime = atoi(splited[1]);
        genBurstFrFl(intarr, bursttime, (prm->k) + 1, burst_count);
        burst_count++;
    }
    increaseThCo();
    pthread_exit(0);
}

int main(int argc, char* argv[]){
    if(argc != 8 && argc != 5){
        printf("Please check your parameters!\nIt should be in form of \"schedule <N> <countB> <minB> <avgB> <minA> <avgA> <ALG>\" or \"schedule <N> <ALG> <F> <FNAME>\"");
        return 1;
    } 
    
    if(argc == 5) { //File read mode 
        if(strcmp(argv[3], "-f") != 0){
            printf("\n Error: PLease check parameters!!\n");
            return 1;
        }
        int n = atoi(argv[1]);
        pthread_t wthreads[n];
        pthread_t sthread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        char ch;
        
        FILE *fp[n];
        for(int i = 0; i < n; i++){
            char* str = NULL;
            int argvlenght = strlen(argv[4]);
            int thcount = n + 1;
            int thdig = 0;
            while(thcount != 0){
                thcount /= 10;
                thdig++;
            }
            int thcountlen = sizeof(int) * thdig; 
            str = malloc(argvlenght + 1 + thcountlen + 4 + 1);
            strcat(str, argv[4]);
            strcat(str, "-");
            char str0[thcountlen + 1];
            sprintf(str0, "%d", (i + 1));
            strcat(str, str0);
            strcat(str, ".txt");
            fp[i] = fopen(str, "r");
            if(fp[i] == NULL){
                printf("\nThe file of the thread no: %d could not be opened. It may not in the same directory!\n", i + 1);
                return 1;
            }
            free(str);
        }
        int endAt = 0; 
        struct sprm prm;
        prm.n = n;
        prm.str = argv[2];
        prm.endAt = &endAt;
        prm.isFileRd = 1;

        if(strcmp(argv[2], "VRUNTIME") == 0)
            initVruntime(n);

        if(pthread_mutex_init(&lock, NULL) != 0){
            printf("\nMutex failed!\n");
            return 1;
        }
        cond = pthread_cond_init(&cv, NULL);
        pthread_create(&sthread, &attr, sthreadmanage, (void*) &prm);
        struct paramfile params[n];
        for(int k = 0; k < n; k++){ 
            srandom(k * 1234567 + time(NULL) + k * 78965 + k * 123);
            params[k].k = k;
            params[k].fp = fp[k];
            pthread_create(&wthreads[k], &attr, burstsFl, (void*) &params[k]);
        }

        for(int k = 0; k < n; k++){
            pthread_join(wthreads[k], NULL);
        }
        pthread_join(sthread, NULL);
        pthread_mutex_destroy(&lock);

        if(strcmp(argv[2], "VRUNTIME") == 0)
            free(thvruntime);
    } else { //Normal mode
        int n = atoi(argv[1]);
        int Bcount = atoi(argv[2]);
        int minB = atoi(argv[3]);
        int avgB = atoi(argv[4]);
        int minA = atoi(argv[5]);
        int avgA = atoi(argv[6]);
        int endAt = n * Bcount;

        struct params param[n];
        for(int j = 0; j < n; j++){
            param[j].bcount = Bcount;
            param[j].minb = minB;
            param[j].avgb = avgB;
            param[j].avga = avgA;
            param[j].mina = minA;
        }
        struct sprm prm;
        prm.n = n;
        prm.str = argv[7];
        prm.endAt = &endAt;
        prm.isFileRd = 0;
        initexparr(n);

        if(strcmp(prm.str, "VRUNTIME") == 0)
            initVruntime(n);

        if(pthread_mutex_init(&lock, NULL) != 0){
            printf("\nMutex failed!\n");
            return 1;
        }
        cond = pthread_cond_init(&cv, NULL);

        pthread_t wthreads[n];
        pthread_t sthread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&sthread, &attr, sthreadmanage, (void*) &prm);
        
        for(int i = 0; i < n; i++){
            srandom(i * 1234567 + time(NULL) + i * 78965 + i * 123);
            param[i].index = i + 1;
            pthread_create(&wthreads[i], &attr, createBurst, (void*) &param[i]);
        }
        for(int k = 0; k < n; k++){
            pthread_join(wthreads[k], NULL);
        }
        pthread_join(sthread, NULL);
        pthread_mutex_destroy(&lock);

        for(int l; l < n; l++){
            printf("\n Average waiting time for the thread %d is %ld", l + 1, experimentArr[l] / Bcount);
        }
        printf("\n");
        free(experimentArr);

        if(strcmp(prm.str, "VRUNTIME") == 0)
            free(thvruntime);
    }
    return 0;
}