# -Parallel-CNN
## 컴퓨터공학 포토폴리오 김지우
### 진현욱 교수님의 시스템프로그래밍을 신청하여 한학기 동안 수강하였습니다.
![KakaoTalk_20201224_122503724](https://user-images.githubusercontent.com/60593969/103407755-23f5dd00-4ba3-11eb-9cd0-96cb4f1c90d7.jpg)   
아직 다전공을 성공하지 못한 상태에서 수강하였기에 일선으로 됬지만, 스스로 다전공을 꼭 할수 있을거라 믿었기에 과감히 전필과목을 수강하였습니다.   
비전공자라 부족하다는 편견을 벗어내기 위해 C를 엄청 공부하였고, 그덕분에 시프 수업 또한 A+을 받을 수 있었습니다.

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
