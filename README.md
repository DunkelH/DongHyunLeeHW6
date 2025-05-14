# cg_code (2)

## Q1. Flat Shading: Triangle-Centric Illumination
#### 개념 요약
Flat shading은 삼각형 면 하나에 대해 단 하나의 색상 값을 계산하고, 그 색을 삼각형 전체에 동일하게 적용한다. 조명 계산은 삼각형 중심점(centroid) 에서 단 한 번 수행되며, 이로 인해 조명이 면 단위로 뚜렷하게 구분되는 느낌을 준다.

렌더링 파이프라인 요약:

View 변환 및 투영 변환을 통해 정점의 클립 공간 위치 계산

삼각형의 screen space bounding box 를 기준으로 래스터화 대상 영역 추출

삼각형 중심점을 월드 좌표계에서 계산하고, 이를 기반으로 면의 법선 벡터(normal) 도 추출

Lambert + Phong 모델 기반으로 diffuse 및 specular 조명을 평가

결과 색상에 대해 감마 보정(γ = 2.2) 적용

삼각형 내부 픽셀에 동일한 색상 적용

#### 핵심 특성

픽셀별 조명 계산이 없으므로 렌더링 비용이 낮다

그러나 경계가 명확히 드러나 조악한 느낌을 줄 수 있음

사용된 normal은 face normal이며, 한 번의 cross product로 정의

## Q2. Gouraud Shading: Vertex-Based Color Interpolation
#### 개념 요약
Gouraud Shading은 각 정점에서 조명을 계산하고, 그 결과 색상(RGB)을 삼각형 내부 픽셀에 바리센트릭 보간으로 분포시킨다. 이 방식은 정점 개수에 따라 부드러운 명암을 제공하지만, specular highlight가 뚜렷하게 표현되지 못하는 한계가 있다.

렌더링 파이프라인 요약:

정점 위치를 월드 공간으로 변환

각 정점에서 로컬 좌표계로부터 normal을 직접 추출 (단위 구 표면의 특징 이용)

각 정점에 대해 Lambert + Phong 조명 모델을 적용 → 정점 RGB 색상 계산

감마 보정(γ = 2.2)은 정점 색상에 직접 적용

삼각형 내의 각 픽셀에 대해 바리센트릭 좌표(u, v, w)를 계산하여 색상을 보간

보간된 색상을 그대로 픽셀에 적용

#### 핵심 특성

정점 수가 많을수록 부드러운 조명 표현 가능

정점 간 색상 보간만 하기 때문에 하이라이트는 희미하거나 생략될 수 있음

감마 보정은 보간 전에 수행해야 물리적으로 정확

## Q3. Phong Shading: Interpolated Normals with Per-Pixel Illumination
#### 개념 요약
Phong Shading은 각 정점에서 normal을 계산한 뒤, 이를 바리센트릭 좌표로 픽셀마다 보간한다. 각 픽셀에서 보간된 normal과 위치를 이용해 조명 모델을 적용하므로, 매우 부드러운 광택과 명암을 표현할 수 있다.

렌더링 파이프라인 요약:

정점 위치 및 normal을 월드 공간으로 변환

normal 계산 시, 단위 구 특성을 이용해 로컬 좌표로부터 normal 추출

각 픽셀에 대해 바리센트릭 좌표(u, v, w)를 계산하여:

정점 위치를 보간 → 픽셀 위치 pos

정점 normal을 보간 → 픽셀 normal

보간된 normal은 normalize 후 Lambert + Phong 조명 계산에 사용

조명 모델 출력 색상에 감마 보정(γ = 2.2) 적용

최종 색상을 OutputImage에 반영

#### 핵심 특성

픽셀당 조명 평가를 통해 부드러운 하이라이트 표현 가능

가장 현실감 있는 결과 제공, 다만 계산량 증가

색상이 아닌 벡터(normal)를 보간하여 물리적 정확도를 확보

