// �ڵ� ��ó - https://computing.llnl.gov/tutorials/pthreads/

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
//    printf("������ %ld�Դϴ�.\n", tid);
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
//        printf("���� �Լ����� %ld ������ ����\n", t);
//        rc = pthread_create(&threads[t], NULL, PrintHello, (void*)t);
//        if (rc) {
//            printf("ERROR - pthread_create() %d\n", rc);
//            exit(-1);
//        }
//    }
//
//    return 0;
//}