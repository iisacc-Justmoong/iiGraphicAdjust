# 소스 책임 분류

공개 헤더와 구현은 `src/` 아래 책임에 따라 함께 배치한다.

- `Core`: 이미지, 연산 계약, 객체 카탈로그와 헤더 전용 객체 인터페이스.
- `Development`: 색상, 톤, 합성, 광학 및 현상 처리.
- `Masks`: 마스크와 국소 보정.
- `Raw`: RAW 처리와 카메라 RAW 필터.
- `Filters`: 흐림, 왜곡, 진단, 렌더링 및 리터치 등 필터.

빌드에서 생성하는 forwarding 헤더와 설치된 공개 헤더는 `<iiGraphicAdjust/...>` 및 `<iiGraphicAdjust/objects/...>` 경로를 유지한다. 생성된 `Export.h`는 계속 빌드 산출물이다. `tests/source_layout.cmake`와 SDK 공통 경계 테스트가 구조 회귀를 검사한다.
