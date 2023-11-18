#include <stdio.h>     /* standard I/O functions                         */
#include <stdlib.h>    /* exit                                           */
#include <string.h>    /* memset                                         */
#include <unistd.h>    /* standard unix functions, like getpid()         */
#include <signal.h>    /* signal name macros, and the signal() prototype */

/* Define global variables */
int ctrl_c_count = 0; // Counter for Ctrl-C signals received
int got_response = 0; // Flag to track if a user response was received
#define CTRL_C_THRESHOLD  5 // Threshold for Ctrl-C signals
#define TIMEOUT_SECONDS 3  // Timeout duration for user response

/* Define the Ctrl-C signal handler */
void catch_int(int sig_num)
{
  /* Increase count and check if threshold was reached */
  ctrl_c_count++;
  if (ctrl_c_count >= CTRL_C_THRESHOLD) {
    char answer[30];

    /* prompt the user to confirm exit or continue */
    printf("\nReally exit? [y/n]: ");
    fflush(stdout);
    alarm(TIMEOUT_SECONDS); // Set a timeout alarm for user response
    fgets(answer, sizeof(answer), stdin); // Read user input
    alarm(got_response); // Cancel the timeout alarm
    if (answer[0] == 'n' || answer[0] == 'N') {
      printf("\nContinuing\n");
      fflush(stdout);
      /* 
       * Reset Ctrl-C counter
       */
      ctrl_c_count = 0;
    }
    else {
      printf("\nExiting...\n");
      fflush(stdout);
      exit(0);
    }
  }
}

/* the Ctrl-Z signal handler */
void catch_tstp(int sig_num)
{
  /* print the current Ctrl-C counter */
  printf("\n\nSo far, '%d' Ctrl-C presses were counted\n\n", ctrl_c_count);
  fflush(stdout);
}

/* STEP - 1 (20 points) */
/* Implement alarm handler - following catch_int and catch_tstp signal handlers */
/* If the user DOES NOT RESPOND before the alarm time elapses, the program should exit */
/* If the user RESPONDEDS before the alarm time elapses, the alarm should be cancelled */
//YOUR CODE

void catch_alarm(int sig_num) // Define the alarm signal handler

{
  /* prompt the user that they took too long before exiting*/
  printf("\n\nUser taking too long to respond. Exiting due to inactivity. . .\n\n");
  fflush(stdout);
  exit(0);
}

int main(int argc, char* argv[])
{
  struct sigaction sa_int, sa_tstp, sa_alarm;
  /* STEP - 2 (10 points) */
  /* clear the memory at sa - by filling up the memory location at sa with the value 0 till the size of sa, using the function memset */
  /* type "man memset" on the terminal and take reference from it */
  /* if the sa memory location is reset this way, then no garbage value can create undefined behavior with the signal handlers */
  //YOUR CODE // 
  // Clear the memory for signal handlers using memset
  memset(&sa_int,0,sizeof(sa_int));
  memset(&sa_tstp,0,sizeof(sa_tstp));
  memset(&sa_alarm,0,sizeof(sa_alarm));


  sigset_t mask_set;  /* used to set a signal masking set. */
  

  /* STEP - 3 (10 points) */
  /* setup mask_set - fill up the mask_set with all the signals to block*/
  //YOUR CODE
  sigfillset(&mask_set); // Setup the signal masking set, filling it with all signals

  
  /* STEP - 4 (10 points) */
  /* ensure in the mask_set that the alarm signal does not get blocked while in another signal handler */
  //YOUR CODE
  sigdelset(&mask_set, SIGALRM); 
  
  
  /* STEP - 5 (20 points) */
  /* set signal handlers for SIGINT, SIGTSTP and SIGALRM */
  //YOUR CODE 
  sa_int.sa_handler = catch_int; //specifies the signal handler
  sa_int.sa_mask = mask_set;   //specifies the mask set
  sigaction(SIGINT, &sa_int, NULL);  //assigns a signal handler based on the contents of sigaction struct

  sa_tstp.sa_handler = catch_tstp;
  sa_tstp.sa_mask = mask_set;
  sigaction(SIGTSTP, &sa_tstp, NULL);

  sa_alarm.sa_handler = catch_alarm;
  sa_alarm.sa_mask = mask_set;
  sigaction(SIGALRM, &sa_alarm, NULL);  
  
  /* STEP - 6 (10 points) */
  /* ensure that the program keeps running to receive the signals */
  //YOUR CODE // !!ADDED!!
  while(1) {
    pause(); // Wait for signals to arrive
  }

  return 0;
}

