#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <time.h>

struct UEinfo{
    int idx;                // UE 식별자
    int timer;              // UE별 타이머 (최종 RA 성공까지 걸리는 딜레이)
    int active;             // 현재 전송 상태 (Not active: 0, MSG 1-2: 1, MSG 3-4: 2)
    int txTime;             // MSG 전송 시간
    int preamble;           // 선택하는 preamble
    int rarWindow;          // RAR window 현재는 5로 설정
    int maxRarCounter;      // 최대 RAR window의 대기 횟수 현재는 10
    int preambleTxCounter;  // Preamble을 변경하여 재전송한 횟수
    int msg2Flag;           // MSG 1-2 성공 여부
    int connectionRequest;  // MSG 3 타이머 현재는 48로 설정
    int msg4Flag;           // MSG 3-4 성공 여부 (결론 RA 성공)
};

void initialUE(struct UEinfo *user, int id);
void activeUE(struct UEinfo *user, int nUE, float pTx, int txTime);
void selectPreamble(struct UEinfo *user, int nPreamble, int time, int backoff);
void preambleCollision(struct UEinfo *user, struct UEinfo *UEs, int nUE, int nPreamble, int time);
void requestResourceAllocation(struct UEinfo *user, int time, int backoff);
void timerIncrease(struct UEinfo *user);
int successUEs(struct UEinfo *user, int nUE);

int main(int argc, char** argv){

    int nUE = 10000;
    struct UEinfo *UE;
    UE = (struct UEinfo *) calloc(nUE, sizeof(struct UEinfo));

    int nPreamble = 64;
    int backoffIndicator = 20;

    for(int i = 0; i < nUE; i++){
        initialUE(UE+i, i);
    }
    
    int time;
    float pTx = 0.1; // 10 %
    int maxTime = 60000; // 60s to ms

    for(time = 0; time < maxTime; time++){
        // MSG 1의 전송을 시도하는 UE들을 결정
        for(int i = 0; i < nUE; i++){
            if((UE+i)->msg4Flag != 1 && (UE+i)->msg2Flag != 1){
                activeUE(UE+i, nUE, pTx, time);
            }
        }
        
        // MSG 1을 전송하는 UE들이 preamble을 선택
        for(int i = 0; i < nUE; i++){
            if((UE+i)->active == 1){
                selectPreamble(UE+i, nPreamble, time, backoffIndicator);
            }
        }
        
        // Preamble collision detection
        for(int i = 0; i < nUE; i++){
            if((UE+i)->active == 1 && (UE+i)->txTime + 2 == time){
                preambleCollision(UE+i, UE, nUE, nPreamble, time);
            }
        }

        // MSG 3 request
        for(int i = 0; i < nUE; i++){
            if((UE+i)->active == 2 && (UE+i)->txTime+2 == time){
                requestResourceAllocation(UE+i, time, backoffIndicator);
            }
        }

        // 전송을 시도하는 UE들의 대기시간 증가
        for(int i = 0; i < nUE; i++)
            timerIncrease(UE+i);

        // if(time == 8){
        //     break;
        // }

        if(successUEs(UE, nUE) == nUE){
            break;
        }
    }

    FILE *fp;
    char fileBuff[1000];
    fp = fopen("Results.txt", "w");

    for(int i = 0; i < nUE; i++){
        // printf("Idx: %d | Timer: %d | Active: %d | txTime: %d | Preamble: %d | RAR window: %d | Max RAR: %d | Preamble reTx: %d | MSG 2 Flag: %d | ConnectRequest: %d | MSG 4 Flag: %d\n",
        // (UE+i)->idx, (UE+i)->timer, (UE+i)->active, (UE+i)->txTime, (UE+i)->preamble, (UE+i)->rarWindow, (UE+i)->maxRarCounter, (UE+i)->preambleTxCounter, (UE+i)->msg2Flag, (UE+i)->connectionRequest, (UE+i)->msg4Flag);
        sprintf(fileBuff, "Idx: %d | Timer: %d | Active: %d | txTime: %d | Preamble: %d | RAR window: %d | Max RAR: %d | Preamble reTx: %d | MSG 2 Flag: %d | ConnectRequest: %d | MSG 4 Flag: %d\n",
        (UE+i)->idx, (UE+i)->timer, (UE+i)->active, (UE+i)->txTime, (UE+i)->preamble, (UE+i)->rarWindow, (UE+i)->maxRarCounter, (UE+i)->preambleTxCounter, (UE+i)->msg2Flag, (UE+i)->connectionRequest, (UE+i)->msg4Flag);

        fputs(fileBuff, fp);
    }

    fclose(fp);

    /*
    struct UEinfo{
    int idx;                // UE 식별자
    int timer;              // UE별 타이머 (최종 RA 성공까지 걸리는 딜레이)
    int active;             // 현재 전송 상태 (Not active: 0, MSG 1-2: 1, MSG 3-4: 2)
    int txTime;             // MSG 전송 시간
    int preamble;           // 선택하는 preamble
    int rarWindow;          // RAR window 현재는 5로 설정
    int maxRarCounter;      // 최대 RAR window의 대기 횟수 현재는 10
    int preambleTxCounter;  // Preamble을 변경하여 재전송한 횟수
    int msg2Flag;           // MSG 1-2 성공 여부
    int connectionRequest;  // MSG 3 타이머 현재는 48로 설정
    int msg4Flag;           // MSG 3-4 성공 여부 (결론 RA 성공)
};
    */

    float averageDelay = 0;

    for(int i = 0; i < nUE; i++){
        averageDelay += (float)(UE+i)->timer;
    }

    printf("Total success time: %dms\n", time);
    printf("Average delay: %lfms\n", averageDelay/nUE);

    return 0;
}

void initialUE(struct UEinfo *user, int id){
    user->idx = id;
    user->active = 0;
    user->txTime = -1;
    user->preamble = -1;
    user->timer = -1;
}

void activeUE(struct UEinfo *user, int nUE, float pTx, int txTime){
    float p = (float)rand() / (float)RAND_MAX;
    if(p < pTx && user->active == 0 && user->msg2Flag != 1){
        user->active = 1;
        user->txTime = txTime;
        user->timer = 0;
        user->msg2Flag = 0;
    }
}

void selectPreamble(struct UEinfo *user, int nPreamble, int time, int backoff){
    // MSG 1 전송을 시작한 UE들
    if(user->active == 1){
        // 처음 전송을 시도하는 UE
        if(user->preamble == -1){
            int tmp = rand() % nPreamble;
            user->preamble = tmp;
            user->rarWindow = 0;
            user->maxRarCounter = 0;
            user->preambleTxCounter = 1;
        }
        // 이전에 전송을 했던 UE들
        else{
            // RAR window를 확인
            if(user->rarWindow >= 5){
                // RAR window가 설정한 값(5) 보다 큰 경우
                user->rarWindow = 0; // RAR window 초기화
                // 지금 preamble의 전송 횟수가 최대인 경우
                if(user->maxRarCounter >= 10){
                    // preamble을 변경하고 backoff만큼 대기
                    user->preamble = rand() % nPreamble;
                    // preamble 전송 횟수
                    user->preambleTxCounter++;
                    // 최대 전송 횟수를 0으로 초기화
                    user->maxRarCounter = 0;
                    // 일단 다음 backoff까지 대기
                    user->txTime = time + (rand() % backoff) + 1;
                }else{
                    // 최대 전송 횟수가 아닌경우 maximum rar counter를 증가
                    user->maxRarCounter++;
                }
            }else{
                user->rarWindow++;
            }
        }
    }
}

// preamble의 충돌 확인
void preambleCollision(struct UEinfo *user, struct UEinfo *UEs, int nUE, int nPreamble, int time){
    int check = 1;
    // MSG 1이 BS에 도달할 때 까지 2 time slot을 기다려야 함
    // 따라서 전송하는 시점에서 2 time slot이 더해진 현재 time이 동일해야 충돌을 확인할 수 있음
    for(int i = 0; i < nUE; i++){
        // MSG 1-2를 보낸 UE들 중
        if((UEs+i)->active == 1){
            // 지금 전송하는 UE와 같은 타임에 전송하는 애들 중
            if((UEs+i)->txTime == user->txTime){
                // 지금 보내고자 하는 애 말고
                if((UEs+i)->idx != user->idx){
                    // preamble 충돌
                    if((UEs+i)->preamble == user->preamble){
                        check = 0;
                    }
                }
                
            }
        }
    }
    if(check == 1){
        user->active = 2;
        user->txTime = time+2;
        user->connectionRequest = 0;
        user->msg2Flag = 1;
    }else{
        user->msg2Flag = 0;
    }
}

void requestResourceAllocation(struct UEinfo *user, int time, int backoff){
    user->connectionRequest++;
    if(user->txTime+2 == time){    
        if(user->connectionRequest < 48){
            float p = (float)rand() / (float)RAND_MAX;
            if(p > 0.1){
                // 90%의 확률로 MSG 3-4 성공
                user->msg4Flag = 1;
                user->active = 0;
            }else{
                // 10%의 확률로 MSG 3-4 실패
                user->txTime = time + (rand() % backoff) + 1;
            }
        }else{
            initialUE(user, user->idx);
        }
    }
}

void timerIncrease(struct UEinfo *user){
    if(user->active != 0)
        user->timer++;
}

int successUEs(struct UEinfo *user, int nUE){
    int success = 0;
    for(int i = 0; i < nUE; i++){
        if((user+i)->msg4Flag == 1){
            success++;
        }
    }
    return success;
}