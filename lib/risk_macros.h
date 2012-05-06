#ifndef RISK_MACROS_H
#define RISK_MACROS_H

#define ASSIGN_ATTACK(E, NUM) (E = NUM)
#define ASSIGN_DEFENSE(E, NUM) (E = (-1) * NUM)
#define I_HAVE(TERR) (TERR > tt_offset && TERR < (tt_offset + tt_per_rank))
#define NEXT_RANK_FROM(R) (R+1 == commSize ? 0 : R+1)
#define PREV_RANK_FROM(R) ((R == 0) ? commSize-1 : R-1)
#define RECV buffer_switch
#define SEND !buffer_switch
#define HEADS 1 //..okayface.jpg
#define TAILS 2
#define COIN_FLIP 2
#define ACC(A, R, C) A[tt_total * R + C]
#define DEBUG() //printf("[%d] DEBUG %d\n", myRank, debug_num++);

#define DEFEND 0
#define ATTACK 1

#define INPUT_BINARY "input/graphbin"
#define HEADER_BYTES 4

#endif