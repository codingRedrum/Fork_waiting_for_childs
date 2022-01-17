#define _GNU_SOURCE
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

volatile sig_atomic_t keep_alarm = 1;
volatile sig_atomic_t nmb_of_chld = 0;
volatile sig_atomic_t sigInit = 0;
int max_czas_zycia_potomkow = 0;
int przerwa_miedzy_procesami = 0;
clock_t start_t, end_t;

/**
 * @brief Funkcja interpretujaca sygnaly     
 *
 * If SA_SIGINFO is specified in sa_flags, then sa_sigaction 
 * (instead of sa_handler) specifies the signal-handling function for signum.  
 * @param sig numer sygnalu
 * @param siginfo strukturą zawierającą różnego rodzaju informacje o źródle sygnału
 * @param ucontext POSIX wymaga, aby była to void *.
 */
void inter_handler(int sig, siginfo_t *siginfo, void *ucontext);

/**
 * @brief funkcja tworzona w procesie potomnym 
 * @param counter okres czasu po ktorym proces potomny wysyla sygnal SIGALRM
 */
void child(unsigned int alarm_lenght);

/**
 * @brief funkcja kontrolujaca odstepy czasowe, przy tworzeniu procesow potomnych
 * @param counter odstep czasowy
 */
void timer(unsigned int counter);

int main(int argc, char **argv)
{
    srand(time(0));
    start_t = clock();
    char *endptr;
    char buf[10];
    max_czas_zycia_potomkow = strtol(argv[1], &endptr, 10);
    przerwa_miedzy_procesami = strtol(argv[2], &endptr, 10);

    // liczba argumentów
    if (argc != 3)
    {
        fprintf(stderr, "%s\n ", strerror(1));
        return 1;
    }

    if (max_czas_zycia_potomkow <= 1 || przerwa_miedzy_procesami <= 1 || *endptr != '\0')
    {
        fprintf(stderr, "arguments must be greater than 1 - exiting\n");
        return 2;
    }

    if (przerwa_miedzy_procesami > 11)
    {
        fprintf(stderr, "przerwa_miedzy_procesami must be smaller than 11 exiting\n");
        return 3;
    }

    if (max_czas_zycia_potomkow > 20)
    {
        fprintf(stderr, "max_czas_zycia_potomkow must be smaller than 21 exiting\n");
        return 3;
    }

    struct sigaction sa;
    sa.sa_handler = 1;
    sa.sa_sigaction = inter_handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);

    printf("IMPORTANT: Kill main process by pressing CTR + C \n\n");

    // tworzymy proces potomny
    while (!sigInit)
    {
        nmb_of_chld++;
        pid_t pid = fork();
        if (pid == 0)
        {
            child(max_czas_zycia_potomkow);
        }
        timer(przerwa_miedzy_procesami);
    }

    while (nmb_of_chld != 0)
    {
        sleep(1);
    };
    sleep(2);
    printf("All child has been reaped - termination. Good bye. \n");
}

void timer(unsigned int counter)
{
    time_t start, end;
    double elapsed;
    time(&start);

    do
    {
        time(&end);
        elapsed = difftime(end, start);
    } while (elapsed < przerwa_miedzy_procesami);
}

void child(unsigned int alarm_lenght)
{
    alarm(alarm_lenght);
    time_t t;
    time(&t);
    srand(time(0));
    int rand_number = rand() % max_czas_zycia_potomkow + 1;
    int pid_t = getpid();
    long long silnia = 1;

    for (int i = rand_number; i > 1; i--)
    {
        silnia *= i;
    }
    printf("Process #%d  |  value: %d  |  factorial: %llu  | creation time: %s", pid_t, rand_number, silnia, ctime(&t));
    while (keep_alarm)
    {
    }
    printf("\t SIGALRM received - employing a handler for the SIGCHLD signal\n");
    exit(rand_number);
}

void inter_handler(int sig, siginfo_t *siginfo, void *ucontext)
{
    if (sig == SIGINT)
    {
        sigInit = 1;
        printf("IMPORTANT: You pressed CTRL + C main loop interupted. Wait for child processes to be reaped. \n");
    }
    if (sig == SIGCHLD)
    {
        time_t t;
        time(&t);
        printf("\t\tEnd #%d  exit value: %d time: %s\n", (int)siginfo->si_pid, (int)siginfo->si_status, ctime(&t));
        nmb_of_chld--;
        //printf("nmb of chld %d \n", nmb_of_chld);
    }
    if (sig == SIGALRM)
    {
        //printf("sigalrm\n");
        keep_alarm = 0;
    }
}
