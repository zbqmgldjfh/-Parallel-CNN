# -Parallel-CNN
## 컴퓨터공학 포토폴리오 김지우
#### 진현욱 교수님의 시스템프로그래밍을 신청하여 한학기 동안 수강하였습니다.
![KakaoTalk_20201224_122503724](https://user-images.githubusercontent.com/60593969/103407755-23f5dd00-4ba3-11eb-9cd0-96cb4f1c90d7.jpg)   
리눅스를 너무 공부해보고 싶었는대 아직 다전공을 성공하지 못하였기에 우선 일선으로 수강

## Parallel CNN 설계
> Parallel CNN중 일부만 구현
### - Convolution layer 구현
### - Pooling layer 구현
<img src = "https://user-images.githubusercontent.com/60593969/103480651-c5399900-4e18-11eb-8659-4f44440d67bc.jpg" width="700px">

## 과정
<img src = "https://user-images.githubusercontent.com/60593969/103772342-b814e800-506c-11eb-83ee-c4cc023a6d23.png" width="700px">

#### 1) 필요한 worker의 수를 파악하고 먼저 worker_pool을 생성한다.    
#### 2) Parent process에서 worker가 준비되었는지 확인후(동기화), 각 worker들에게 id값을 전달한다.   
#### 3) 각 worker들은 id값을 받고 Filter르 사용하여 연산한다. 이후 대기   
#### 4) Parent process에서 각 worker들에게 signal 전달   
#### 5) worker들이 Parent process에게 결과값 전송   
#### 6) 이를 Pooling layer에서도 반복   
#### 7) 최종 결과를 Parent process에서 받은후 output 출력   

## Convolutional layer
<img src = "https://user-images.githubusercontent.com/60593969/103774701-5787aa00-5070-11eb-9a7c-a30e6460ef27.jpg" width="650px">

#### 1) NxN matrix에서 3x3의 지정된 filter를 이용하여 Convolution연산을 진행함 
#### 2) result_matrix는 (n-2)x(n-2) 가 된다. 이는 필요한 Cworker의 수와 동일.
             
## Pooling layer
<img src = "https://user-images.githubusercontent.com/60593969/103774695-56ef1380-5070-11eb-9e91-dc8629a76928.jpg" width="650px">

#### 1) max-pooling 연산을 해줌. 각 사이즈는 2x2로 고정
#### 2) input_matrix가 nxn이면 output으로는 (n/2)x(n/2)가 필요. 이는 필요한 Pworker의 수와 동일.

## ID값
<img src = "https://user-images.githubusercontent.com/60593969/103899247-7736d500-5139-11eb-94c8-522326cbc2b9.png" width="650px">

**ID값은 각 원소의 행과 열의 index를 이용하여 고유한 값을 부여하였습니다.**

## Message Structure

**데이터 송수신을 위한 구조체 선언**

```C
struct msg_matrix_3x3_st  // 3x3 matrix 전달(main->worker로)
{
    long id;
    int value[3][3];
};

struct msg_matrix_2x2_st // 2x2 matrix 전달(main->worker로)
{
    long id;
    int value[2][2];
};

struct msg_ret_st // 결과값 받아오기 (worker->main으로 )
{
    long id;
    int value;
};
```
## Message Queue API
   -> POSIX Message Queue 이용    
   -> 이러한 IPC 사용을 위해서 multi_process로 구성

```C
// 매번 mq_open을 하려면 attr값을 넘겨야 하는대 이에대한 반복을 줄이기 위해 wrapper함수를 만듬
mqd_t ku_mq_open(char *mqname, int msg_size) // message queue wrapper funtion
{
    struct mq_attr attr;
    mqd_t mq_des;
    attr.mq_maxmsg = 10; // 메시지큐의 최대수는 10개
    attr.mq_msgsize = msg_size;
    mq_des = mq_open(mqname, O_CREAT | O_RDWR, 0600, &attr);
    if (mq_des < 0)
    {
        perror("mq_open()");
        exit(0);
    }
    return mq_des; // 메시지큐 id값 반환
}
// 위에서 만든 ku_mq_open을 내장한 sned함수
void ku_mq_send(char *mqname, char *data, int msg_size, unsigned int id) // send용 wrapper 함수
{
    mqd_t mq_des = ku_mq_open(mqname, msg_size); // 큐를 열고
    if (mq_send(mq_des, (char *)&data, msg_size, id) == -1) // 메시지를 보내는
    {
        perror("mq_send");
        exit(0);
    }
    mq_close(mq_des);
}

void ku_mq_receive(char *mqname, char *data, int msg_size, unsigned int id) // receive용 wrapper 함수
{
    unsigned int msgid;
    mqd_t mq_des = ku_mq_open(mqname, msg_size); // 큐를 열고

    if (mq_receive(mq_des, (char *)&data, msg_size, &msgid) == -1) // 메시지를 받는
    {
        perror("mq_receive");
        exit(0);
    }

    mq_close(mq_des);
}
```

## Signal handler
```C
void signalhandler()  // 단순 signal_interrupt 발생용
{
}
```

각 process가 동기화를 마친후 pause()를 통해 각 worker를 정지시켜둔다.   
data 통신 준비를 위해 signal을 전송하여 pause()에서 벗어나 send 또는 receive를 하도록 호출한다.    
handler는 단순히 pause()에서 벗어나기위한 용도이다.
