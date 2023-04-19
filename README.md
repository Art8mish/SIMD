# Отчет по лабораторной работе SIMD (МФТИ 2023)

## Цель работы
Исследование возможностей процессора оптимизации программ с использованием принципа SIMD.

## Введение
`SIMD` (Single Instruction, Multiple Data - одиночный поток команд, множественный поток данных) - принцип вычислений, позволяющий обеспечить параллелизм выполнения команд. Архитектуры современных процессоров позволяют выполнять несколько инструкций одновременно, параллельно при помощи специальных расширенных `xmm` регистров. Эти возможности и используются в ходе оптимизации вычислений с помощью SIMD инструкций. В работе используются наборы команд `SSE4` и `AVX2`. Стоит отметить, что эти технологии доступны не на всех процессорах.

Конвейерная обработка команд процессором имеет несколько недостатоков, один из них - риск загрузки и использования. Он возникает из-за зависимости следующей команды от результата предыдущей, поэтому процессор условно приостанавливает конвейер, пока не будет выполнена предыдущая инструкция. Чтобы сократить количество таких зависимостей применяется векторная обработка данных (отсюда и название - одна операция, множественный поток данных) с помощью специальных наборов команд (в этой работе представлены `SSE4` и `AVX2`).

## Ход работы
### Часть 1. Множество Мандельброта
#### Теоритическая справка
**Множество Мандельброта** - это множество точек на плоскости таких, что для рекурсивных уравнений $x_{i+1} = x_i^2 - y_i^2 + x_0$ и $y_{i+1} = 2 * x_i * y_i+ y_0$ выполняется неравенство $R = (x^2 + y^2)^{1/2} >= R_{max}$, то есть функция $f(x_0, y_0) -> N$ ставит $(x_0, y_0)$ в соответствие число N - количество операций для нахождения таких x и y, что выполняется неравенство выше. Это множество является фрактальным.

#### Алгоритм и его оптимизация
Вычислять множество будем в пределах (-2, 2) с разрешением 1000x1000.

##### Код неоптимизированного алгоритма
```С++
float dx = 1 / ppe;
float dy = 1 / ppe;

int xMax  = 1000;
int yMax  = 1000;
int nMax  = 256;

float rMax2 = 100;

float x0 = -2;
float y0 =  2;

float x = x0;
float y = y0;

for  (int yi = 0; yi < yMax; yi++)
{
    for (int xi = 0; xi <= xMax; xi++)
    {
        x = x0;
        y = y0;
        int n = 0;
        for (; n < nMax; n++)
        {
            float ty = x * y + x * y + y0;
            x = x * x - y * y + x0;
            y = ty;

            float r2 = y * y + x * x; 
            if (r2 >= rMax2)
                break;
        }
        N = n;
        x0 += dx;
    }

    x0 = -2;
    y0 -= dy;
}
```
Полученное значение N будет использоваться для вычисления цвета пикселя по формуле: `RGB = ((N % 4) * 64, N, N)`. Получим изображенияе множества Мандельброта.

<img src="/pics/mandelbrot.png" style="height: 370px;"/>

Теперь оптимизируем данный код, используя набор команд `AVX2` из библиотеки `<immintrin.h>`. Ознакомиться с командами можно на сайте: `https://www.laruence.com/sse`.

##### Код оптимизированного алгоритма
```С++
__m256 dx = _mm256_set1_ps(1 / ppe);
__m256 dy = _mm256_set1_ps(1 / ppe);

int xMax  = 1000 / 8;
int yMax  = 1000;
int nMax  = 256;

__m256 rMax2 = _mm256_set1_ps(100);

__m256 _01234567   = _mm256_set_ps(0, 1, 2, 3, 4, 5, 6, 7);
__m256 dx_01234567 = _mm256_mul_ps(dx, _01234567);
__m256 dx_8        = _mm256_mul_ps(dx, _mm256_set1_ps(8));

__m256 x0 = _mm256_set1_ps(-2 - x_off);
       x0 = _mm256_add_ps(x0, dx_01234567);
__m256 y0 = _mm256_set1_ps(2 + y_off);

for  (int yi = 0; yi < yMax; yi++)
{
    for (int xi = 0; xi < xMax; xi++)
    {
        __m256 x = x0;
        __m256 y = y0;
        
        __m256 cntr  = _mm256_setzero_ps();
        __m256 index = _mm256_set1_ps(1);
        __m256 mask  = _mm256_set1_ps(0xFFFFFFFF);

        for (int n = 0; n < nMax; n++)
        {
            __m256 xy = _mm256_mul_ps(x, y);
            __m256 x2 = _mm256_mul_ps(x, x);
            __m256 y2 = _mm256_mul_ps(y, y);

            y = _mm256_add_ps(_mm256_add_ps(xy, xy), y0);
            x = _mm256_add_ps(_mm256_sub_ps(x2, y2), x0);

            __m256 r2 = _mm256_add_ps(_mm256_mul_ps(y, y), _mm256_mul_ps(x, x));
                 mask = _mm256_cmp_ps(r2, rMax2, _CMP_LT_OS);

            cntr = _mm256_add_ps(cntr, _mm256_and_ps(index, mask));

            if (_mm256_movemask_ps(mask) == 0)
                break;
        }

        x0 = _mm256_add_ps(x0, dx_8);
    }

    x0 = _mm256_set1_ps(-2);
    x0 = _mm256_add_ps(x0, dx_01234567);
    y0 = _mm256_sub_ps(y0, dy);
}
```

Тип `__m256` представляет из себя 8 float'ов по 4 байта, упакованных в массив. Операции выполняются с несколькими значениями одновременно (в одном цикле вычисляется 8 значений N для восьми значений $(x_0, y_0)$ ). Это происходит потому, что почти исключена зависимость каждой следующей операции над одним из значений от результата предыдущей, тем самым достигается параллелилизм.

Так как прорисовка изображения с помощью графической библиотеки SFML затрачивает достаточное количество времени, чтобы повлиять на результаты, для чистоты эксперимента отключим её при измерениях. Получим результаты времени работы программы с оптимизациями -O1, -O2 и без них. Проведём по 10 измерений для каждого условия.

##### Время работы программы

Средние значения:
| Flag | Without optimization | With AVX2 |
|------|:--------------------:|----------:|
| None |       0.3364 с       | 0.0784 с  |
| -O1  |       0.1683 с       | 0.0164 с  | 
| -O2  |       0.1650 с       | 0.0069 с  |

Из данных результатов видно, что мы получили при -O2 производительность выросла в 23 раза, что невозможно, так как счёт производился лишь по 8-ми элементам одновременно, поэтому и прирост скорости должен быть не больше, чем в 8 раз. Причина в том, что из-за того, что прорисовка во время тестов отключена и вычисляемое значение N нигде не используется, оптимизация с флагом -O2 провелась так, что вычисления просто не производятся, так как из результат нигде не используется. Ассемблерный код можно посмотреть на сайте `https://godbolt.org/`. 

Скомпилированный ассемблерный код при -O2 в тестовом режиме выглядит следующим образом:

<img src="/pics/asm.png" style="height: 300px;"/>




Из результатов видно, что использование набора команд AVX2 с флагом оптимизации -O2 увеличило скорость выполнения программы в 48 раз. Разница между результатами с и без AVX2 для разных флагов оптимизации варьируется от 4 в случае None до 23 раз при -O2 (около 10 при -O1).

### Часть 2. Alpha Blending
#### Теоритическая справка
**Alpha Blending** или Альфа-смешение - это процесс комбинирования двух изображений на экране с учётом их альфа-каналов. Альфа-канал характеризует прозрачность изображения. В данной работе мы будем накладывать одно изображение на другое. 

#### Алгоритм и его оптимизация
Нужно поместить изображение кота с прозрачностью на стол моего друга, увлеченного игрой в шахматы.

<img src="/alpha_blending/pics/AskhatCat.bmp" name="кот" style="height: 150px;"/>
<img src="/alpha_blending/pics/yasin.bmp" name="друг" style="height: 300px;"/>

##### Код неоптимизированного алгоритма
```C++
BYTE a = front[3];
sf::Color color = sf::Color{(BYTE)((front[0] * a + back[0] * (255 - a)) >> 8),
                            (BYTE)((front[1] * a + back[1] * (255 - a)) >> 8),
                            (BYTE)((front[2] * a + back[2] * (255 - a)) >> 8)};
```
Здесь `а` - прозрачность (альфа-канал) накладываемого изображения. `Color` - это 4 байта отвечающие за цвет пикселя (по байту на компоненту цвета и байт на альфа-канал).

Получим итоговое изображение:

<img src="/pics/yasin+kot.png" name="кот-друг" style="height: 300px;"/>

Теперь оптимизируем данный алгоритм с помощью набора команд `SSE4` из библиотеки `<nmmintrin.h>`. 
`SSE4` отличается от `AVX2` урезанным набором команд, это просто более старая технология.

##### Код оптимизированного алгоритма
```C++
    __m128i fr;
    memcpy(&fr, front, sizeof(__m128i));                                        //fr = [r3 g3 b3 a3 | r2 g2 b2 a2 | r1 g1 b1 a1 | r0 g0 b0 a0]

    __m128i bk;                                                                 //bk = [r3 g3 b3 a3 | r2 g2 b2 a2 | r1 g1 b1 a1 | r0 g0 b0 a0]
    memcpy(&bk, back, sizeof(__m128i));

    __m128i _0 = _mm_set1_epi16(0);
    __m128i FR = (__m128i) _mm_movehl_ps((__m128) _0, (__m128) fr);             //FR = [0 0 0 0 | 0 0 0 0 | r3 g3 b3 a3 | r2 g2 b2 a2]
    __m128i BK = (__m128i) _mm_movehl_ps((__m128) _0, (__m128) bk);             //BK = [0 0 0 0 | 0 0 0 0 | r3 g3 b3 a3 | r2 g2 b2 a2]
    
    fr = _mm_cvtepu8_epi16(fr);                                                 //fr = [0 r1 0 g1 | 0 b1 0 a1 | 0 r0 0 g0 | 0 b0 0 a0]
    bk = _mm_cvtepu8_epi16(bk);                                                 //bk = [0 r1 0 g1 | 0 b1 0 a1 | 0 r0 0 g0 | 0 b0 0 a0]
    FR = _mm_cvtepu8_epi16(FR);                                                 //FR = [0 r3 0 g3 | 0 b3 0 a3 | 0 r2 0 g2 | 0 b2 0 a2]
    BK = _mm_cvtepu8_epi16(BK);                                                 //BK = [0 r3 0 g3 | 0 b3 0 a3 | 0 r2 0 g2 | 0 b2 0 a2]

    const __m128i movA_0 = _mm_set_epi8(0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF,
                                        0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF);
    const __m128i movA   = _mm_set_epi8(_z, 14, _z, 14, _z, 14, _z, 14, 
                                        _z, 6,  _z, 6,  _z, 6,  _z, 6);
    __m128i a = _mm_and_si128(_mm_shuffle_epi8(fr, movA), movA_0);              //a = [0 a1 0 a1 | 0 a1 0 a1 | 0 a0 0 a0 | 0 a0 0 a0]
    __m128i A = _mm_and_si128(_mm_shuffle_epi8(FR, movA), movA_0);              //a = [0 a3 0 a3 | 0 a3 0 a3 | 0 a2 0 a2 | 0 a2 0 a2]

    fr = _mm_mullo_epi16(fr, a);                                                //fr *= a
    FR = _mm_mullo_epi16(FR, A);                                                //FR *= a

    __m128i _255 = _mm_set1_epi16(255);                                         
    bk = _mm_mullo_epi16(bk, _mm_sub_epi16(_255, a));                           //bk *= 255 - a
    BK = _mm_mullo_epi16(BK, _mm_sub_epi16(_255, A));                           //BK *= 255 - A

    __m128i sum = _mm_add_epi16(fr, bk);                                        //sum = [R1 r1 G1 g1 | B1 b1 A1 a1 | R0 r0 G0 g0 | B0 b0 A0 a0]
    __m128i SUM = _mm_add_epi16(FR, BK);                                        //SUM = [R3 r3 G3 g3 | B3 b3 A3 a3 | R2 r2 G2 g2 | B2 b2 A2 a2]

    const __m128i movSum_0 = _mm_set_epi8(0,    0,    0,    0,    0,    0,    0,    0,
                                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);               
    const __m128i movSum   = _mm_set_epi8(_z, _z, _z, _z, _z, _z, _z, _z,
                                          15, 13, 11,  9,  7,  5,  3,  1);      //sum = sum >> 8
    sum = _mm_and_si128(_mm_shuffle_epi8(sum, movSum), movSum_0);               //sum = [0 0 0 0 | 0 0 0 0 | R1 G1 B1 A1 | R0 G0 B0 A0]
    SUM = _mm_and_si128(_mm_shuffle_epi8(SUM, movSum), movSum_0);               //sum = [0 0 0 0 | 0 0 0 0 | R3 G3 B3 A3 | R2 G2 B2 A2]


    __m128i color = (__m128i) _mm_movelh_ps((__m128)sum, (__m128)SUM);          //color = [R3 G3 B3 A3 | R2 G2 B2 A2 | R1 G1 B1 A1 | R0 G0 B0 A0]
```

Тип `__m128i` представляет из себя упакованные в 16 байт 4 знаковых int'a, соответственно, по 4 байта каждый.

Аналогично, как и в 1 части работы, для чистоты эксперимента отключим прорисовку и замерим время выполнения программы с теми же условиями. В ходе проведения эксперимента были получены результаты по порядку слишком малые, чтобы оценить прирост в скорости, поэтому искусственно увеличим время работы программы, поместив функцию вычисления в цикл.

##### Время работы программы
| SSE4 | Flag | t, c  | t, c  | t, c  | t, c  | t, c  | t, c  | t, c  | t, c  | t, c  | t, c  |
|:----:|:----:|:------|-------|-------|-------|-------|-------|-------|-------|-------|------:|
|  -   | None | 1.114 | 1.222 | 1.212 | 1.229 | 1.308 | 1.269 | 1.173 | 1.164 | 1.219 | 1.204 |
|  +   | None | 1.385 | 1.444 | 1.490 | 1.417 | 1.291 | 1.344 | 1.402 | 1.441 | 1.364 | 1.285 |
|  -   | -O1  | 0.931 | 0.978 | 0.941 | 0.930 | 0.967 | 0.963 | 0.992 | 1.008 | 0.985 | 0.961 |
|  +   | -O1  | 0.457 | 0.465 | 0.467 | 0.465 | 0.464 | 0.482 | 0.454 | 0.462 | 0.466 | 0.451 |
|  -   | -O2  | 0.751 | 0.764 | 0.768 | 0.776 | 0.793 | 0.795 | 0.777 | 0.767 | 0.775 | 0.750 |
|  +   | -O2  | 0.434 | 0.436 | 0.428 | 0.433 | 0.435 | 0.434 | 0.436 | 0.436 | 0.439 | 0.429 |


Средние значения:
| Flag | Without optimization | With SSE4 |
|------|:--------------------:|----------:|
| None |       1.2114 с       | 1.3863 с  |
| -O1  |       0.9656 с       | 0.4633 с  | 
| -O2  |       0.7716 с       | 0.4340 с  |


Заметим, что время выполнения программы с использованием SIMD инструкций без указания флага оптимизации больше, чем без SSE4. Это связано с тем, что компилятор 'не видит' возможностей оптимизации с использованием `xmm` регистров.
Для -O1 и -O2 получены значения увеличения скорости работы программы в около 2 раза. Разница между самой медленной версией (без флагов и SSE4) - 3 раза. Результаты не такие показательные, как в первом эксперименте с множеством Мандельброта, так как использовались технологии помедленнее, но и такие значения показывают огромный прирост производительности в большинстве случаев.

## Вывод
В ходе данной работы были исследованы возможности процессора оптимизации программ с использованием принципа SIMD. Были получены результы по приросту от 4 до 48 раз в зависимости от флагов оптимизации в первой части (множество Мандельброта) и от 2 до 3 раз во второй (Alpha Blending). Это показывает, что принцип параллелизма позволяет достичь огромного прироста в производительности.

## Источники и литература
1. SIMD:
    - https://ru.wikipedia.org/wiki/SIMD
    - https://habr.com/ru/articles/440566/
    - https://russianblogs.com/article/13881254588/
    - https://www.laruence.com/sse

2. Множество Мандельброта:
    - https://ru.wikipedia.org/wiki/Множество_Мандельброта

3. Alpha Blending:
    - https://ru.wikipedia.org/wiki/Альфа-канал
    - https://jenyay.net/Programming/Bmp (руководство по BMP формату)

4. Графическая библиотека SFML
    - https://www.sfml-dev.org/


Литература:
- Randal E. Bryant and David R. O'Hallaron "Computer Systems: A Programmer's Perspective"
