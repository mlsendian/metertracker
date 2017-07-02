#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <wiringPi.h>
#include <hiredis/hiredis.h>

#define BLIPS_KEY 	"blips"
#define BLIP_BUCKET_SIZE	60 //seconds
#define	LED_PIN		0

static volatile unsigned int interruptCounter = 0 ;

void interrupter (void) {
    ++interruptCounter ;
    //printf("hit\n");
}

int gpio_setup() {
    if (wiringPiSetup () < 0) {
        fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno)) ;
        return 1 ;
    }

    if (wiringPiISR (LED_PIN, INT_EDGE_FALLING, &interrupter) < 0) {
        fprintf (stderr, "Unable to setup ISR: %s\n", strerror (errno)) ;
        return 1 ;
    }
    return 0;
}

redisContext * redis_setup(const char *hostname, int port) {
    redisReply *reply;
    redisContext *c;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    /* PING server */
    reply = redisCommand(c,"PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);
    return c;
}


void record_time(redisContext *c) {
    struct timeval timestamp;
    char ts_str[17];
    redisReply *reply;

    if (gettimeofday(&timestamp, NULL) != 0) {
        perror("Time failed");
        return;
    }

    reply = redisCommand(c,"ZADD " BLIPS_KEY " %08d%08d %08d.%08d", timestamp.tv_sec, timestamp.tv_usec, timestamp.tv_sec, timestamp.tv_usec);
    freeReplyObject(reply);

    printf("foo: %08d.%08d\n", timestamp.tv_sec, timestamp.tv_usec);
}

int main(int argc, char **argv) {
    unsigned int myCounter = 0;
    redisContext *c;
    const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 6379;
    int i;

    if (gpio_setup() != 0) {
        return 1;
    }

    c = redis_setup(hostname, port);

    for (;;) {
        printf ("Waiting ... ") ; fflush (stdout) ;
        while (myCounter == interruptCounter)
            delay (50) ;

        record_time(c);
//        myCounter++;
        myCounter = interruptCounter;
    }

    /* Disconnects and frees the context */
    redisFree(c);

    return 0;
}
