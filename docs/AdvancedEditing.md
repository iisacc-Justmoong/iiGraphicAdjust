# 내용 인식·합성·미리보기 객체 계약

이 문서는 iiGraphicAdjust의 독립적인 CPU 이미지 편집 객체의 입력과 결과를 정의한다.
기존 이미지 계약과 같이 RGBA는 정규화된 직선 알파 sRGB이며, 입력은 호출 중에만
대여하고 결과는 독립 소유한다. 빈 이미지, 크기 불일치, 유효하지 않은 수치는
명시적인 오류로 반환한다. 실행 순서와 동시 접근은 호출자가 관리한다.

## 내용 인식

- `ContentAwareInpaintSolver`는 호출자가 선택한 마스크 안을 주변과 유사한 알려진
  픽셀 표본으로 채운다. 생성형 모델은 사용하지 않으며 마스크 밖을 보존한다.
  마스크 값이 0보다 큰 픽셀을 선택하고, 값이 1보다 작으면 원본과 채운 색을
  알파를 곱한 상태에서 보간한다. 선택하지 않은 원본 픽셀만 표본으로 사용하며,
  사용할 표본이 없는 경우 실패한다. `patchRadius`와 `maxCandidates`로 탐색 범위와
  후보 수를 제한한다.
- `SeamConstraintMask`는 보호 마스크와 제거 우선 마스크를 부호 있는 가중치로
  합친다. 양수는 보호, 음수는 제거를 선호하며 강제 경계가 아니다.
- `ContentAwareRescaleSolver`는 이미지 에너지와 제약 가중치에 따라 수직·수평
  솔기를 제거하거나 삽입한다. 삽입 픽셀은 솔기와 이웃을 알파를 곱한 상태에서
  보간한다. 출력 크기와 작업 예산은 호출자가 명시할 수 있다.

## 레이어와 합성

- `LayerRegistrationSolver`는 두 이미지의 가시 영역이 충분히 겹치는 정수 픽셀
  이동을 탐색한다. 이미지는 같은 크기여야 하며 반환 이동량은 이동 이미지 좌표에
  더해 기준 이미지 좌표를 얻는다. 불투명도가 0인 픽셀은 비교에서 제외된다.
- `LayerStackBlendSolver`는 동일 크기 레이어를 아래에서 위 순서로 기존 블렌드
  모드와 불투명도를 사용해 합성한다.
- `SkyReplacementCompositor`는 호출자가 제공한 하늘 커버리지로 원본과 교체 하늘을
  보간한다. 색상은 알파를 곱한 상태에서 보간하며 자동 하늘 선택은 수행하지 않는다.
- `SubjectEdgeMatteRefiner`는 호출자 마스크를 원본 영상으로 유도하여 가장자리를
  다듬는다. `ColorDecontaminationPass`는 호출자가 제공한 배경과 피사체 커버리지로
  부분 경계의 색 오염을 선형 sRGB에서 완화한다. 배경 RGB는 알려진 색 표본으로
  취급하며 배경 알파는 사용하지 않는다. 원본 불투명도는 변경하지 않는다.

## 미리보기와 필터

- `PuppetWarpPreviewState`, `PerspectiveWarpPreviewState`,
  `AdaptiveWideAnglePreviewState`, `LensCorrectionPreviewState`, `LiquifyPreviewState`,
  `VanishingPointPreviewState`는 기존 해당 연산의 설정 스냅샷을 소유한다. 설정 교체,
  실행 취소, 다시 실행 및 현재 스냅샷 재렌더를 지원한다. 성공한 편집과 이력 이동은
  단조 증가 리비전과 새 프레임을 반환하며, 실패 시 상태를 변경하지 않는다.
- `FilterGalleryPipeline`은 순서가 있는 기존 갤러리 효과를 같은 편집 이력 계약으로
  실행한다. `FilterPreviewCache`는 호출자가 부여한 입력·파이프라인 식별자와 리비전,
  실제 입력 픽셀 스냅샷을 확인하여 오래된 결과를 무효화한다. 저장 공간과 동시 접근은
  호출자가 관리한다.
- `Float32PreviewToneMapper`는 기존 HDR 톤 매핑 계약을 사용한다. 입력은 음이 아닌
  유한 선형 sRGB float32 RGB 및 정규화된 알파이며 1을 넘는 하이라이트를 허용한다.

객체는 GUI, 백그라운드 스레드 또는 타이머를 소유하지 않는다. 호출자는 편집 결과의
리비전을 전달받아 캐시를 조회하거나 폐기하고 즉시 새 프레임을 표시할 수 있다.
이는 다른 실행 객체를 내부에서 호출하지 않고 실시간 갱신 순서를 외부에서 제어한다.
캐시 식별자는 동시에 존재하는 서로 다른 원본과 파이프라인에 재사용하지 않는다.
같은 파이프라인에서 교체·실행 취소·다시 실행을 수행하면 리비전은 증가한다.

## C++와 Python 사용

C++에서는 `#include <iiGraphicAdjust>` 또는 각 공개 헤더를 포함하고
`iiGraphicAdjust::objects` 이름 공간에서 객체를 생성한다. `execute`의 성공값은
호출자가 소유하며 오류는 `std::expected`로 전달된다. 미리보기의 `Replace`는 새
설정과 렌더가 모두 성공한 뒤 이력을 확정한다. 새 편집은 기존 다시 실행 분기를
제거하고 이력 개수 제한을 넘으면 가장 오래된 스냅샷부터 버린다.
이 문서가 정의하는 복수 입력·편집 상태 객체의 목록은 기존 309개 함수 래퍼를
열거하는 `operationCatalog()`와 별도로 관리한다.

네이티브 빌드는 기본적으로 CPython 확장 `iigraphicadjust`를 제공한다. Python의
`Image`, `Mask`, 옵션 및 결과의 리스트는 값으로 복사되며 리스트를 수정한 뒤에는
해당 속성에 다시 할당한다. Python 실패는 `ValueError`로 전달된다. Python의
미리보기 `execute`는 `image`, `settings`, `revision`, `can_undo`, `can_redo` 키를
가진 사전을 반환한다. `FilterPreviewCache.execute`는 `hit`, `image`를 반환한다.
빌드와 CMake 패키지는 iiSDKCore 0.20의 표준 라이브러리 기반 `Contracts` 구성 요소를
공통 기반으로 요구한다. 파일 저장 및 앱의 실행 순서는 여전히 호출자가 관리한다.
Windows의 경우 애플리케이션은 확장을 가져올 때 설치 `bin/`을 DLL 검색 경로에
명시적으로 추가해야 한다.

```python
import iigraphicadjust as ga

source = ga.Image(2, 1, [ga.Rgba(1, 0, 0, 1), ga.Rgba(0, 1, 0, 1)])
state = ga.LensCorrectionPreviewState()
settings = ga.LensCorrectionOptions()
settings.scale = 120
frame = state.execute(source, ga.PreviewAction.REPLACE, settings)
undone = state.execute(source, ga.PreviewAction.UNDO)
```

현재 솔버는 CPU의 결정적 근사 처리이며 자동 피사체 선택, 생성형 채우기, 회전·원근
정렬 또는 특정 상용 편집기의 픽셀 호환성을 제공하지 않는다. 인페인팅 비용은 선택
픽셀과 후보 수 및 패치 면적에, 솔기 비용은 크기 변경량과 이미지 면적에 비례한다.
호출자는 대형 이미지를 작업 스레드에서 실행하고 예산을 조정해야 한다.
