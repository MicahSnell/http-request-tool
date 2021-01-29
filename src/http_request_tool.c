/**
 * File: http_request_tool.c
 *
 * Author: Micah Snell
 * Date: 10/28/20
 *
 * This program uses sockets to send a HTTP request to a user specified url.
 * The program then prints the response to the screen. The url is given at the
 * command line by using the flag '--url' and following it with the url.
 *
 * An optional parameter can be used to send multiple requests to the given url.
 * When the program is run with this parameter, the output will be statistics
 * about the requests and responses. The parameter is given at the command line
 * using the flag '--profile' and following it with a positive integer.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define BUF_SIZE 4096

// a linked list of time taken to send message and receive a response
struct Node {
  long time;
  struct Node *next;
};

struct Node *timeList = NULL;
int listSize = 0;

// a linked list of error codes from responses
struct Error {
  char *code;
  struct Error *next;
};

struct Error *errorCodeList = NULL;

void insertNodeSorted(long);
void printList();
float findMedian();

void insertError(char *);
void printErrorCodes();

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "\nMinimal usage: ./request --url <url.to.send.a.request>\n"
                    "\nUse --help for detailed usage.\nEx: ./request --help\n\n"
                  );
    exit(EXIT_FAILURE);
  }

  if (strcmp(argv[1], "--help") == 0) {
    printf("\nDESCRIPTION\n\n"
           "  This tool sends http GET requests via TCP sockets to the\n"
           "  specified URL, and the response is printed to the console.\n"
           "\nPARAMETERS\n\n"
           "  --url <url.to.send.a.request>\n"
           "      This parameter is the only required parameter. It specifies\n"
           "      the URL that you want to send a GET request to. The url can\n"
           "      include a /path to load specific pages from a host. If none\n"
           "      is given a default / will be requested from the host.\n"
           "        Ex: ./request --url example.com\n\n"
           "  --profile <a positive integer>\n"
           "      This parameter will make the program send the specified\n"
           "      number of requests to the specified URL.\n"
           "        Ex: ./request --url example.com --profile 3\n\n"
           "  --help\n"
           "      This displays the help menu you are currently reading!\n"
           "        Ex: ./request --help\n\n");
    exit(EXIT_SUCCESS);
  }

  struct addrinfo hints;
  struct addrinfo *result, *rp;

  int sockFileDesc, s, i;

  size_t len;
  ssize_t nread, minBytes, maxBytes;

  struct timeval whatTime;
  long start, stop, elapsed, minTime, maxTime, totalTime;
  float median;
  int numRequest = 1, successfulReqs;

  char *port = "80";
  char *host;
  char *slash;
  char request[BUF_SIZE] = "GET ";
  char response[BUF_SIZE];
  char statusCode[3];

  if (strcmp(argv[1], "--url") == 0) {
    slash = strchr(argv[2], '/');

    if (slash == NULL) {
      slash = "/";            // set to / default
      strcat(request, slash);  // add / after GET in http request
      host = argv[2];
    }
    else {
      strcat(request, slash); // add /path after GET in http request

      *slash = '\0';    // remove /path, only the host url remains in argv[2]
      host = argv[2];
    }

    // build the rest of the http request
    strcat(request, " HTTP/1.1\r\nHost: ");
    strcat(request, host);
    strcat(request, "\r\nConnection: close\r\n\r\n");
  }

  if (argc == 5 && strcmp(argv[3], "--profile") == 0) {
    numRequest = atoi(argv[4]);
    if (numRequest < 1) {
      printf("Invalid argument, defaulting to 1.\n");
      numRequest = 1;
    }
  }

  successfulReqs = numRequest; // decrements on receipt of error code

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;      //IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = 0;

  for (i = 0; i < numRequest; ++i) {
    // create a linked list of matches to the given host, pointed to by result
    s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
      fprintf(stderr, "getaddrinfo: %s\n"
                      "Ensure the --url flag is used before the URL\n\n",
                      gai_strerror(s));
      exit(EXIT_FAILURE);
    }

    // try to open a socket with each node from the result linked-list
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      sockFileDesc = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

      if (sockFileDesc == -1)
        continue;

      if (connect(sockFileDesc, rp->ai_addr, rp->ai_addrlen) != -1)
        break; // success

      close(sockFileDesc);
    }

    if (rp == NULL) {
      fprintf(stderr, "Could not connect\n"
                      "Ensure the --url flag is used before the URL\n\n");
      exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    len = strlen(request) + 1; // + 1 for null terminator

    // record start time and send request
    gettimeofday(&whatTime, NULL);
    start = (long)whatTime.tv_sec * 1000 + (long)whatTime.tv_usec / 1000;
    if (send(sockFileDesc, request, len, 0) != len) {
      fprintf(stderr, "Failed to send request.\n\n");
      exit(EXIT_FAILURE);
    }

    // receive response and record stop time
    nread = recv(sockFileDesc, response, BUF_SIZE, 0);
    if (nread == -1) {
      fprintf(stderr, "Failed to receive response.\n\n");
      exit(EXIT_FAILURE);
    }
    gettimeofday(&whatTime, NULL);
    stop = (long)whatTime.tv_sec * 1000 + (long)whatTime.tv_usec / 1000;

    if (argc == 3 && strcmp(argv[1], "--url") == 0) {
      printf("Received %zd bytes\n", nread);
      printf("Response:\n%s\n\n", response);
    }

    close(sockFileDesc);

    if (argc == 5 && strcmp(argv[3], "--profile") == 0) {
      statusCode[0] = response[9];
      statusCode[1] = response[10];
      statusCode[2] = response[11];
      if (strcmp(statusCode, "200") != 0) {
        --successfulReqs;
        insertError(statusCode);
      }

      elapsed = stop - start;
      insertNodeSorted(elapsed);

      if (i == 0) {
        minTime = maxTime = totalTime = elapsed;
        minBytes = maxBytes = nread;
      }
      else {
        if (elapsed < minTime)
          minTime = elapsed;
        else if (elapsed > maxTime)
          maxTime = elapsed;

        if (nread < minBytes)
          minBytes = nread;
        else if (nread > maxBytes)
          maxBytes = nread;

        totalTime += elapsed;
      }
    }
  }

  if (argc == 5 && strcmp(argv[3], "--profile") == 0) {
    median = findMedian();
    printf("Number of requests: %i\n", numRequest);
    printf("Fastest time (ms): %ld\n", minTime);
    printf("Slowest time (ms): %ld\n", maxTime);
    printf("Average time (ms): %.1f\n", (float)totalTime / numRequest);
    printf("Median  time (ms): %.1f\n", median);
    printList();
    printf("Requests succeeded: %.2f\%\n",
           (float)(successfulReqs / numRequest) * 100);
    printErrorCodes();
    printf("Smallest response: %zd bytes\n", minBytes);
    printf("Largest  response: %zd bytes\n", maxBytes);
    printf("\n\n");
  }

  free(timeList);
  free(errorCodeList);
  exit(EXIT_SUCCESS);
}

/**
 * @desc insertNodeSorted creates a sorted linked list from least to greatest,
 *   of time taken to send and receive a request from a host.
 * @param value - A time in milliseconds to add to the list.
 * @returns n/a
 */
void insertNodeSorted(long value) {
  struct Node *precursor = NULL,
              *cursor = timeList;

  while (cursor != NULL && cursor->time < value) {
    precursor = cursor;
    cursor = cursor->next;
  }

  struct Node *newNode = (struct Node*)malloc(sizeof(struct Node));
  newNode->time = value;
  newNode->next = cursor;

  if (cursor == timeList)
    timeList = newNode;
  else
    precursor->next = newNode;

  ++listSize;
}

/**
 * @desc findMedian finds the median value of the linked list of time taken to
 *   send/receive a request/response.
 * @param none
 * @returns median
 */
float findMedian() {
  int position = 1;
  struct Node *cursor = timeList;

  if (listSize % 2 == 0) {              // median is an average of two numbers
    long leftMiddle, rightMiddle;

    while (cursor != NULL && position < (listSize / 2)) {
      ++position;
      cursor = cursor->next;
    }
    leftMiddle = cursor->time;

    if (cursor->next != NULL)
      cursor = cursor->next;
    rightMiddle = cursor->time;

    return (float)(leftMiddle + rightMiddle) / 2;
  }
  else if (listSize % 2 == 1) {         // median is in the middle of the list
    while (cursor != NULL && position <= (listSize / 2)) {
      ++position;
      cursor = cursor->next;
    }

    return (float)cursor->time;
  }

  return -1; // error
}

/**
 * @desc insertError adds http error codes that were received from server
 *   responses to a linked list.
 * @param errorCode An error code to add to the list.
 * @returns n/a
 */
void insertError(char *errorCode) {
  struct Error *newError = (struct Error*)malloc(sizeof(struct Error));
  newError->code = errorCode;
  newError->next = NULL;

  if (errorCodeList != NULL) {
    newError->next = errorCodeList;
    errorCodeList = newError;
  }
  else
    errorCodeList = newError;
}

/**
 * @desc printList prints the list of times.
 * @param none
 * @returns n/a
 */
void printList() {
  struct Node *cursor = timeList;

  printf("Times: ");
  while (cursor != NULL) {
    printf("%ld ", cursor->time);
    cursor = cursor->next;
  }
  printf("\n");
}

/**
 * @desc printErrorCodes prints the list of error codes.
 * @param none
 * @returns n/a
 */
void printErrorCodes() {
  struct Error *cursor = errorCodeList;

  printf("Error codes: ");
  while (cursor != NULL) {
    printf("%s ", cursor->code);
    cursor = cursor->next;
  }
  printf("\n");
}
