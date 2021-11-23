#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdarg.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg
{
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
/************ STUDENTS NEED TO MODIFY BELOW CODE************/

// Pre Define
#define A 0
#define B 1
#define TIMEOUT 20
#define BUF_SZ 10000
#define WINDOW_SZ 10

int sender_buf_upper;
int receiver_buf_upper;
int window_left; // Window Left
int window_right; // Window Right

char sender_buffer[BUF_SZ][20];
char receiver_buffer[WINDOW_SZ][20];

int A_seqnum;
int B_acknum;
int left_seqnum;

void inform(const char* __func, const char* format, ...);

int calc_cSum(struct pkt packet)
{
    int c_sum = 0;
    int len = sizeof(packet.payload) / sizeof(char);
    c_sum += packet.seqnum;
    c_sum += packet.acknum;

    for(int i = 0; i < len; ++i)
        c_sum += packet.payload[i];

    return c_sum;
}

int checksum(struct pkt packet)
{
    return calc_cSum(packet) == packet.checksum ? 1 : 0;
}

struct pkt make_packet(int seqnum, char payload[20])
{
    struct pkt packet;
    packet.seqnum = seqnum;
    packet.acknum = 0;
    for(int i = 0; i < 20; i++)
        packet.payload[i] = payload[i];
    packet.checksum = calc_cSum(packet);          
    return packet;
}

void send_packet(int AorB, int seqnum, char payload[20])
{
    const char* sender = A == AorB ? "A_output" : "B_output";
    inform(sender, "Send Pkt | Seq: %d | Msg: %.20s", seqnum, payload);
    struct pkt packet = make_packet(seqnum, payload);
    tolayer3(AorB, packet);
}

void send_range(int AorB, int seq_start, int shift){
    int ptr = window_right;
    int last = (ptr + shift + BUF_SZ) % BUF_SZ;
    int seqnum = seq_start;
    while(ptr != last){
        send_packet(AorB, seqnum, sender_buffer[ptr]);
        ptr = (ptr + 1) % BUF_SZ;
        seqnum = (seqnum + 1) % (WINDOW_SZ + 1); 
    }
}

struct pkt make_ack(int acknum)
{
    struct pkt packet;
    packet.seqnum = acknum;
    packet.acknum = acknum;
    for(int i = 0; i < 20; i++)
        packet.payload[i] = 0;
    packet.checksum = calc_cSum(packet);
    return packet;
}

void send_ack(int AorB, int acknum)
{
    const char* sender = A == AorB ? "A_input" : "B_input";
    inform(sender, "Send ACK[%d]", acknum);
    struct pkt packet = make_ack(acknum);
    tolayer3(AorB, packet);
}

int is_ACK_valid(struct pkt *packet, int base, int right)
{
    int shift = 0;
    for(int i = base; i != right; i = (i + 1) % (WINDOW_SZ + 1)){
        shift++;
        if(packet->acknum == i)
            return shift;
    }
    return 0;
}

int is_Seq_valid(struct pkt *packet, int base, int right)
{
    int shift = 0;
    for(int i = base; i != right; i = (i + 1) % (WINDOW_SZ + 1)){
        shift++;
        if(packet->seqnum == i)
            return shift;
    }
    return 0;
}

int get_sender_window_shift(uint32_t start, uint32_t end)
{
    assert(start <= end);
    int shift = 0;
    for(int i = start; i < end; i++){
        if(sender_buffer[i][0] == '\0')
            shift++;
        else
            return shift;
    }
    return shift;
}

int get_receiver_window_shift(uint32_t start, uint32_t end)
{
    int shift = 0;
    for(int i = start; i < end; i++){
        if(receiver_buffer[i][0] != '\0')
            shift++;
        else
            return shift;
    }
    return shift;
}

void clean_pkt(char (*buffer)[20], uint32_t loc)
{
    buffer[loc][0] = '\0';
}

int get_next_Seqnum(const int seqnum, const int shift)
{
    return (seqnum + shift) % (WINDOW_SZ + 1);
}

int get_last_Seqnum(const int seqnum)
{
    return (seqnum + WINDOW_SZ) % (WINDOW_SZ + 1);
}

void inform(const char* __func, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[%s]: ", __func);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void cache_sender_msg(struct msg* msg)
{
    memcpy(sender_buffer[sender_buf_upper], msg->data, sizeof(msg->data));
    sender_buf_upper = (sender_buf_upper + 1) % BUF_SZ;
}

void cache_receiver_msg(char payload[20], int seq_shift)
{
    memcpy(receiver_buffer[seq_shift], payload, sizeof(char) * 20);
    receiver_buf_upper = (receiver_buf_upper + 1) % WINDOW_SZ;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    printf("------------------------------\n");
    if(sender_buf_upper == window_left){
        inform(__FUNCTION__, "Start Timer");
        starttimer(A, TIMEOUT);
    }
    cache_sender_msg(&message);
    if((sender_buf_upper - window_left + WINDOW_SZ) % WINDOW_SZ <= WINDOW_SZ){
        send_packet(A, A_seqnum, message.data);    
        A_seqnum = get_next_Seqnum(A_seqnum, 1);
        window_right = (window_right + 1) % BUF_SZ;
    } else {
        inform(__FUNCTION__, "Window is full, BUF the msg: %.20s", message.data);
    }
}

/* need be completed only for extra credit */
void B_output(struct msg message)
{
    inform(__FUNCTION__, "Got Msg");
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    inform(__FUNCTION__, "Recv ACK[%d]", packet.acknum);    
    int window_range = (window_right - window_left + 1 + BUF_SZ) % BUF_SZ;
    // Case1: CheckSum Failed
    if(!checksum(packet)){
        inform(__FUNCTION__, "Checksum Failed, Dropped the packet");
        return;
    }

    // Case2: ACK is Wrong
    int ack_shift = is_ACK_valid(&packet, left_seqnum, left_seqnum + window_range - 1);
    if(ack_shift == 0){
        inform(__FUNCTION__, "Recv ACK[%d], Ignore", packet.acknum);
    }

    // Case3: ACK is Correct
    else {
        inform(__FUNCTION__, "Right ACK Num, Timer Stopped", packet.acknum);
        stoptimer(A);

        uint32_t loc = (window_left + ack_shift - 1 + BUF_SZ) % BUF_SZ;
        clean_pkt(sender_buffer, loc);
        int shift = get_sender_window_shift(window_left, sender_buf_upper);

        window_left = (window_left + shift) % BUF_SZ;
        left_seqnum = get_next_Seqnum(left_seqnum, shift);

        if(sender_buf_upper > window_right && shift > 0){
            int shift_right = (sender_buf_upper >= (window_right + shift)) ? shift : sender_buf_upper - window_right;
            inform(__FUNCTION__, "Slide right & Send Cached Msg");
            send_range(A, A_seqnum, shift_right);
            window_right = (window_right + shift_right) % BUF_SZ;
        } 

        if (window_left != window_right)
            starttimer(A, TIMEOUT);
    }
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
    // A Time Out send the packet n
    inform(__FUNCTION__, "Resend Seq[%d]", left_seqnum);
    send_packet(A, left_seqnum, sender_buffer[window_left]);
    inform(__FUNCTION__, "Start Timer");
    starttimer(A, TIMEOUT);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    A_seqnum = 0;
    left_seqnum = 0;
    sender_buf_upper = 0;
    window_left = 0;
    window_right = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    int last_seqnum = get_last_Seqnum(B_acknum);
    inform(__FUNCTION__, "Recv Seq[%d] | Msg: %.20s", packet.seqnum, packet.payload);    
    
    // Case 1: CheckSum Failed
    // Dropped the packet
    if(!checksum(packet)){
        inform(__FUNCTION__, "CheckSum failed"); 
        return;
    } 

    int seq_shift = is_Seq_valid(&packet, B_acknum, (B_acknum + WINDOW_SZ) % (WINDOW_SZ + 1));
    // Case 2: Recv Invalid ACK[n] (n in [B_acknum-N, B_acknum-1])
    // Send ACK(n)
    if(seq_shift == 0){
        inform(__FUNCTION__, "ACK Out of Window Seq[%d]", packet.seqnum);
        send_ack(B, packet.seqnum);
    }
    // Case 3: Recv ACK[n] (n in [B_acknum, B_acknum+N-1])
    // Send ACK(n)
    // If 
    else {
        send_ack(B, packet.seqnum);
        cache_receiver_msg(packet.payload, seq_shift - 1);
        int shift = get_receiver_window_shift(0, WINDOW_SZ - 1);
        B_acknum = get_next_Seqnum(B_acknum, shift);
        for(int i = 1; i <= shift; i++){
            uint32_t loc = (i + seq_shift - 2 + WINDOW_SZ)%WINDOW_SZ;
            tolayer5(B, receiver_buffer[loc]);
            clean_pkt(receiver_buffer, loc);
        }
        
    }
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
    printf("B timer time Out\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
    B_acknum = 0;
    receiver_buf_upper = 0;
}

/************ STUDENTS NEED TO MODIFY ABOVE CODE************/
/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
    - emulates the transmission and delivery (possibly with bit-level corruption
        and packet loss) of packets across the layer 3/4 interface
    - handles the starting/stopping of a timer, and generates timer
        interrupts (resulting in calling students timer handler).
    - generates message to be sent (passed from later 5 to 4)
THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOULD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you definitely should not have to modify
******************************************************************/

struct event
{
    float evtime;       /* event time */
    int evtype;         /* event type code */
    int eventity;       /* entity where event occurs */
    struct pkt *pktptr; /* ptr to packet (if any) assoc w/ this event */
    struct event *prev;
    struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;   /* for my debugging */
int nsim = 0;    /* number of messages from 5 to 4 so far */
int nsimmax = 0; /* number of msgs to generate, then stop */
float g_time = 0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void init(int argc, char **argv);
void generate_next_arrival(void);
void insertevent(struct event *p);

int main(int argc, char **argv)
{
    struct event *eventptr;
    struct msg msg2give;
    struct pkt pkt2give;

    int i, j;
    char c;

    init(argc, argv);
    A_init();
    B_init();

    while (1)
    {
        eventptr = evlist; /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next; /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2)
        {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        g_time = eventptr->evtime; /* update time to next event time */
        if (eventptr->evtype == FROM_LAYER5)
        {
            if (nsim < nsimmax)
            {
                if (nsim + 1 < nsimmax)
                    generate_next_arrival(); /* set up future arrival */
                /* fill in msg to give with string of same letter */
                j = nsim % 26;
                for (i = 0; i < 20; i++)
                    msg2give.data[i] = 97 + j;
//                msg2give.data[19] = 0;
                if (TRACE > 2)
                {
                    printf("          MAINLOOP: data given to student: ");
                    for (i = 0; i < 20; i++)
                        printf("%c", msg2give.data[i]);
                    printf("\n");
                }
                nsim++;
                if (eventptr->eventity == A)
                    A_output(msg2give);
                else
                    B_output(msg2give);
            }
        }
        else if (eventptr->evtype == FROM_LAYER3)
        {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A) /* deliver packet by calling */
                A_input(pkt2give);       /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr); /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT)
        {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else
        {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

    terminate:
    printf(
            " Simulator terminated at time %f\n after sending %d msgs from layer5\n",
            g_time, nsim);
}

void init(int argc, char **argv) /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();

    if (argc != 6)
    {
        printf("usage: %s  num_sim  prob_loss  prob_corrupt  interval  debug_level\n", argv[0]);
        exit(1);
    }

    nsimmax = atoi(argv[1]);
    lossprob = atof(argv[2]);
    corruptprob = atof(argv[3]);
    lambda = atof(argv[4]);
    TRACE = atoi(argv[5]);
    printf("-----  Selective Repeat Network Simulator Version 1.1 -------- \n\n");
    printf("the number of messages to simulate: %d\n", nsimmax);
    printf("packet loss probability: %f\n", lossprob);
    printf("packet corruption probability: %f\n", corruptprob);
    printf("average time between messages from sender's layer5: %f\n", lambda);
    printf("TRACE: %d\n", TRACE);

    //srand((unsigned)time(NULL)); /* init random number generator */
    srand(1);
    sum = 0.0;   /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75)
    {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(1);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    g_time = 0.0;              /* initialize g_time to 0.0 */
    generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand(void)
{
    double mmm = RAND_MAX;
    float x;          /* individual students may need to change mmm */
    x = rand() / mmm; /* x should be uniform in [0,1] */
    return (x);
}

/********************* EVENT HANDLING ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival(void)
{
    double x, log(), ceil();
    struct event *evptr;
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
    /* having mean of lambda        */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = g_time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}

void insertevent(struct event *p)
{
    struct event *q, *qold;

    if (TRACE > 2)
    {
        printf("            INSERTEVENT: time is %lf\n", g_time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist; /* q points to header of list in which p struct inserted */
    if (q == NULL)
    { /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else
    {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL)
        { /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist)
        { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else
        { /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

void printevlist(void)
{
    struct event *q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next)
    {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
               q->eventity);
    }
    printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB /* A or B is trying to stop timer */)
{
    struct event *q, *qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", g_time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;        /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist)
            { /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            }
            else
            { /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB /* A or B is trying to stop timer */, float increment)
{
    struct event *q;
    struct event *evptr;

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", g_time);
    /* be nice: check to see if timer is already started, if so, then  warn */
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
        {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = g_time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB /* A or B is trying to stop timer */, struct pkt packet)
{
    struct pkt *mypktptr;
    struct event *evptr, *q;
    float lastime, x;
    int i;

    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob)
    {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2)
    {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
               mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
    /* finally, compute the arrival time of packet at the other end.
       medium can not reorder, so make sure packet arrives between 1 and 10
       time units after the latest arrival time of packets
       currently in the medium on their way to the destination */
    lastime = g_time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();

    /* simulate corruption: */
    if (jimsrand() < corruptprob)
    {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z'; /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
    int i;
    if (TRACE > 2)
    {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }
}
