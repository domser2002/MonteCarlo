#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#define ITERATIONS 10000
#define LINE_WIDTH 8

void usage(char *progname)
{
    fprintf(stderr,"USAGE: %s 0 < N < 30\n",progname);
    exit(EXIT_FAILURE);
}

float monte_carlo()
{
    srand(getpid());
    int circle_points=0;
    for(int i=0;i<ITERATIONS;i++)
    {
        float a=2.0;
        float x = ((float)rand()/(float)(RAND_MAX)) * a - 1.0;
        float y = ((float)rand()/(float)(RAND_MAX)) * a - 1.0;
        float d = x*x+y*y;
        if(d<=1) circle_points++;
    }
    return 4*(float)circle_points/(float)ITERATIONS;
}

void wait_children()
{
    pid_t pid;
    for (;;)
    {
        pid = wait(NULL);
        if (pid <= 0)
        {
            if (errno == ECHILD)
                break;
            perror("wait");
            exit(EXIT_FAILURE);
        }
    }
}

void child_work(int idx,float *results,char *logs)
{
    float pi = monte_carlo();
    char buf[LINE_WIDTH+1];
    results[idx] = pi;
    snprintf(buf,LINE_WIDTH+1,"%7.5f\n",pi);
    memcpy(logs + idx * LINE_WIDTH,buf,LINE_WIDTH);
}

void parent_work(int n,float *results,char *logs)
{
    float pi = 0.0;
    wait_children();
    for(int i=0;i<n;i++)
        pi += results[i];
    pi /= n;
    printf("PI is equal to %f\n",pi);
}

void create_n_children_and_map_memory(int n)
{
    int fd;
    float *results;
    char *logs;
    if((results = (float*)mmap(NULL,n * sizeof(float),PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0)) == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    if((fd = open("./log.txt",O_CREAT | O_RDWR | O_TRUNC,-1)) == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if(ftruncate(fd,n*LINE_WIDTH))
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    if((logs = (char*)mmap(NULL,n * LINE_WIDTH,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0)) == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    if(close(fd))
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    for(int i=0;i<n;i++)
    {
        pid_t pid = fork();
        switch (pid)
        {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
            case 0:
                child_work(i,results,logs);
                exit(EXIT_SUCCESS);
            default:
                break;
        }
    }
    parent_work(n,results,logs);
    if(munmap(results,n * sizeof(float)) == -1)
    {
        perror("munmap");
        exit(EXIT_FAILURE);
    }
    if(msync(logs,n * LINE_WIDTH,MS_SYNC))
    {
        perror("msync");
        exit(EXIT_FAILURE);
    }
    if(munmap(logs,n * LINE_WIDTH) == -1)
    {
        perror("munmap");
        exit(EXIT_FAILURE);
    }
}

int main(int argc,char **argv)
{
    int N;
    if(argc != 2) usage(argv[0]);
    N = atoi(argv[1]);
    if(N <= 0 || N >= 30) usage(argv[0]);
    create_n_children_and_map_memory(N);
    return EXIT_SUCCESS;
}