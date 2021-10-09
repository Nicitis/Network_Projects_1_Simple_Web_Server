// 코드 출처 - https://computing.llnl.gov/tutorials/pthreads/

//#define HAVE_STRUCT_TIMESPEC
//#include <pthread.h>
//#include <stdio.h>
//#include <stdlib.h>
//
//#define NUM_THREADS     5
//
//void* PrintHello(void* threadid)
//{
//    long tid;
//    tid = (long)threadid;
//    printf("스레드 %ld입니다.\n", tid);
//    pthread_exit(0);
//
//    return NULL;
//}
//
//int main(int argc, char* argv[])
//{
//    pthread_t threads[NUM_THREADS];
//    int rc;
//    long t;
//    for (t = 0; t < NUM_THREADS; t++) {
//        printf("메인 함수에서 %ld 스레드 생성\n", t);
//        rc = pthread_create(&threads[t], NULL, PrintHello, (void*)t);
//        if (rc) {
//            printf("ERROR - pthread_create() %d\n", rc);
//            exit(-1);
//        }
//    }
//
//    return 0;
//}