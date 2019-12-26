#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define A_ROWS 20
#define A_COLS 10

#define B_ROWS 10
#define B_COLS 20

int  mtr_A[A_ROWS][A_COLS];
int  mtr_B[B_ROWS][B_COLS];
int  mtr_C[A_ROWS][B_COLS];

enum state
{
    READY,
    MUL,
    FINISH,
    UNKNOWN,
    ERR
};


typedef struct
{
    enum state state;
    unsigned int i,j;
    pthread_mutex_t mut;
    pthread_cond_t cond;
} Thread_params;

Thread_params *thread_params=NULL;

void *body(void *args)
{
    unsigned int row, col;
   
    Thread_params *params=(Thread_params *)args;

    while(1)
    {
        pthread_mutex_lock(&params->mut);

        if(params->state == FINISH)
        {
            pthread_mutex_unlock(&params->mut);
            return NULL;
        }

        params->state = READY;

        pthread_cond_wait(&params->cond, &params->mut);

        if(params->state != MUL)
        {
            pthread_mutex_unlock(&params->mut);
            continue;
        }

        row=params->i;
        col=params->j;

        pthread_mutex_unlock(&params->mut);

        {
            int i;
             mtr_C[row][col]=0;

            for(i=0; i < A_COLS; i++)
            {
                mtr_C[row][col] += mtr_A[row][i] * mtr_B[i][col];
            }
        }
        
    }

    return NULL;

}

int main(int argc,char **argv)
{
    int num_threads=0;
    int i;
    int finish_counter=0;
    
    unsigned int row,col;

    unsigned int current_i=0, current_j=0;

    pthread_t *threads=NULL;

    if(argc<2)
    {
        fprintf(stderr,"num_threads in args\n");
        return 1;
    }

    num_threads=atoi(argv[1]);
    if(num_threads<=0)
    {
        fprintf(stderr,"We see %d threads\n But it too little",num_threads);
        return 1;
    }

    for(row=0; row < A_ROWS; row++)
    {
        for(col=0; col < A_COLS; col++)
        {
           // mtr_A[row][col]=row+col;
           mtr_A[row][col]=2;
        }
    }

    for(row=0; row < B_ROWS; row++)
    {
        for(col=0; col < B_COLS; col++)
        {
            mtr_B[row][col]=2;
        }
    }

    threads=(pthread_t *)malloc(sizeof(pthread_t)*num_threads);
    if(threads == NULL)
    {
        return 2;
    }

    thread_params=(Thread_params *)malloc(sizeof(Thread_params)*num_threads);
    if(thread_params == NULL)
    {
        return 2;
    }
    
    for(i=0;i<num_threads;i++)
    {
        if(pthread_mutex_init(&thread_params[i].mut, NULL) == -1)
        {
            return 3;
        }

        if(pthread_cond_init(&thread_params[i].cond, NULL) == -1)
        {
            return 3;
        }

        thread_params[i].i=0;
        thread_params[i].j=0;
        thread_params[i].state=UNKNOWN;

        if(pthread_create(&threads[i],NULL,body,&thread_params[i]) == -1)
        {
            return 3;
        }
    }

    while(1)
    {
        if(current_i == A_ROWS)
        {
            break;
        }

        for(i=0;i<num_threads; i++)
        {
            pthread_mutex_lock(&thread_params[i].mut);

            if(thread_params[i].state == READY)
            {
                if(current_i == A_ROWS)
                {
                    thread_params[i].state=FINISH;
                    finish_counter++;
                }
                else
                {
                    thread_params[i].i=current_i;
                    thread_params[i].j=current_j;
                    thread_params[i].state=MUL;
                }

                pthread_cond_signal(&thread_params[i].cond);
                
                if(current_i != A_ROWS)
                {
                    current_j++;
                    if(current_j == B_COLS)
                    {
                        current_i++;
                        current_j=0;
                    }
                }
            }
            pthread_mutex_unlock(&thread_params[i].mut);
        } /* end for */
    } /* end  while */

    while(finish_counter < num_threads)
    {
        for(i=0;i<num_threads;i++)
        {
            pthread_mutex_lock(&thread_params[i].mut);
            if(thread_params[i].state == READY)
            {
                thread_params[i].state=FINISH;
                pthread_cond_signal(&thread_params[i].cond);
                finish_counter++;
            }
            pthread_mutex_unlock(&thread_params[i].mut);
        }
    }

    for(i=0;i<num_threads;i++)
    {
            pthread_join(threads[i],NULL);
            pthread_mutex_destroy(&thread_params[i].mut);
            pthread_cond_destroy(&thread_params[i].cond);
    }

    free(threads);
    free(thread_params);

    for(row=0; row < A_ROWS; row++)
    {
        for(col=0; col < B_COLS; col++)
        {
            printf("%d ",mtr_C[row][col]);
        }
        putchar('\n');
    }

    return 0;
}

