믹서에서 소리에 이펙트를 적용할 때 :

- Compressor : 입력 신호의 순간 레벨을 샘플 단위로 추적. 레벨이 임계치를 넘으면 ratio 기반하여 gain을 줄이고, 넘지 않으면 통과시키기. attack/release 시간은 지수 곡선으로 반응하도록. 선형으로 해버리면 gain이 갑자기 꺾여서 이상하게 들린다고 함.
- Delay : 에코 딜레이를 구현해야함. 최대 2초 길이의 링 버퍼에 신호를 기록.
- Reverb : JUCE의 reverb 구현을 사용
- Dynamic EQ:
