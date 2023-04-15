
#include "common.h"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <WinDef.h>
#include <wingdi.h>

#include <emmintrin.h>
#include <immintrin.h>

#include <time.h>

#define LAB_MOD
//#undef  LAB_MOD

int PrintCatPic();

int CalcPointmap   (sf::VertexArray *pointmap, const sf::Uint8 *front, const sf::Uint8 *back);
int CalcPointmapSSE(sf::VertexArray *pointmap, const sf::Uint8 *front, const sf::Uint8 *back);

sf::Color CalcAlpha (const sf::Uint8 *front, const sf::Uint8 *back);

__m128i CalcAlpha_4 (const sf::Uint8 *front, const sf::Uint8 *back);
__m128i CalcAlphaSSE(const sf::Uint8 *front, const sf::Uint8 *back);


static const int BK_WIDTH  = 800;
static const int BK_HEIGHT = 600;
static const int FR_WIDTH  = 235;
static const int FR_HEIGHT = 126;

int main(int argc, char *argv[])
{
    int err = PrintCatPic();
    ERR_CHCK(err, ERROR_PRINT_MANDELBROT);

    return SUCCESS;
}

int CalcPointmapSSE(sf::VertexArray *pointmap, const sf::Uint8 *front, const sf::Uint8 *back)
{
    int bk_pix_am = BK_WIDTH * BK_HEIGHT;
    int fr_pix_am = FR_WIDTH * FR_HEIGHT;

    int step = 4;
    for (int bk_i = 0, fr_i = 0; bk_i < bk_pix_am; bk_i += step) //bk_rgbs_amnt
    {
        int x = bk_i % BK_WIDTH;
        int y = bk_i / BK_WIDTH;

        sf::Color color[step];
        if ((y < 550) && (y > 550 - FR_HEIGHT) && (x > 251) && (x <= 480))
        {
            __m128i sse_arr = CalcAlphaSSE(&front[fr_i * 4], &back[bk_i * 4]);
            //__m128i sse_arr = CalcAlpha_4(&front[fr_i * 4], &back[bk_i * 4]);

            
            // for (int j = 0; j < step; j++)
            // {
            //     color[j] = CalcAlpha(&front[(fr_i + j) * 4], &back[(bk_i + j) * 4]);
            //     color[j] = sf::Color{((sf::Uint8 *)&sse_arr)[j * 4], 
            //                          ((sf::Uint8 *)&sse_arr)[j * 4 + 1], 
            //                          ((sf::Uint8 *)&sse_arr)[j * 4 + 2]};
            // }
           

            fr_i += step;

            if (x == 480)
                fr_i += 3;
        }

        else
        {
            #ifndef LAB_MOD
            for (int j = 0; j < step; j++)
                color[j] = sf::Color{back[(bk_i + j) * 4], back[(bk_i + j) * 4 + 1], back[(bk_i + j) * 4 + 2]};
            #endif
        }
            
        #ifndef LAB_MOD
        for (int v_i = 0; v_i < step; v_i++)
        { 
            (*pointmap)[bk_i + v_i].position = sf::Vector2f(x, y);
            (*pointmap)[bk_i + v_i].color = color[v_i];

            x = (bk_i + v_i + 1) % BK_WIDTH;
            y = (bk_i + v_i + 1) / BK_WIDTH;
        }
        #endif
    }

    return SUCCESS;
}

int CalcPointmap(sf::VertexArray *pointmap, const sf::Uint8 *front, const sf::Uint8 *back)
{
    int bk_pix_am = BK_WIDTH * BK_HEIGHT;
    int fr_pix_am = FR_WIDTH * FR_HEIGHT;

    int step = 1;
    for (int bk_i = 0, fr_i = 0; bk_i < bk_pix_am; bk_i += step) //bk_rgbs_amnt
    {
        int x = bk_i % BK_WIDTH;
        int y = bk_i / BK_WIDTH;

        sf::Color color;
        if ((y < 550) && (y > 550 - FR_HEIGHT) && (x > 251) && (x <= 486))
        {
            color = CalcAlpha(&front[fr_i * 4], &back[bk_i * 4]);
            fr_i += step;
        }

        else
        {
            #ifndef LAB_MOD
                color = sf::Color{back[bk_i * 4], back[bk_i * 4 + 1], back[bk_i * 4 + 2]};
            #endif
        }
            
        #ifndef LAB_MOD
        (*pointmap)[bk_i].position = sf::Vector2f(x, y);
        (*pointmap)[bk_i].color    = color;
        #endif
    }

    return SUCCESS;
}

sf::Color CalcAlpha(const sf::Uint8 *front, const sf::Uint8 *back)
{
    BYTE a = front[3];
    sf::Color color = sf::Color{(BYTE)((front[0] * a + back[0] * (255 - a)) >> 8),
                                (BYTE)((front[1] * a + back[1] * (255 - a)) >> 8),
                                (BYTE)((front[2] * a + back[2] * (255 - a)) >> 8)};

    return color;
}

#define SIMD_LOG(name, arr)                                                                                         \
            fprintf(log_f, "%s[%2d %2d %2d %2d | %2d %2d %2d %2d | %2d %2d %2d %2d | %2d %2d %2d %2d]\n", name,     \
            (arr)[0], (arr)[1], (arr)[2],  (arr)[3],  (arr)[4],  (arr)[5],  (arr)[6],  (arr)[7],                    \
            (arr)[8], (arr)[9], (arr)[10], (arr)[11], (arr)[12], (arr)[13], (arr)[14], (arr)[15])   

#define ENTER() fprintf(log_f, "\n")      

#define _z 0

//debug function
__m128i CalcAlpha_4(const sf::Uint8 *front, const sf::Uint8 *back)
{
    sf::Uint8 a1 = front[3];
    sf::Uint8 a2 = front[7];
    sf::Uint8 a3 = front[11];
    sf::Uint8 a4 = front[15];
    __m128i color = _mm_set_epi8((BYTE)(0),
                                 (BYTE)((front[14] * a4 + back[14] * (255 - a4)) >> 8),
                                 (BYTE)((front[13] * a4 + back[13] * (255 - a4)) >> 8),
                                 (BYTE)((front[12] * a4 + back[12] * (255 - a4)) >> 8),
                                 (BYTE)(0),
                                 (BYTE)((front[10] * a3 + back[10] * (255 - a3)) >> 8),
                                 (BYTE)((front[9]  * a3 + back[9]  * (255 - a3)) >> 8),
                                 (BYTE)((front[8]  * a3 + back[8]  * (255 - a3)) >> 8),
                                 (BYTE)(0),
                                 (BYTE)((front[6]  * a2 + back[6]  * (255 - a2)) >> 8),
                                 (BYTE)((front[5]  * a2 + back[5]  * (255 - a2)) >> 8),
                                 (BYTE)((front[4]  * a2 + back[4]  * (255 - a2)) >> 8),
                                 (BYTE)(0),
                                 (BYTE)((front[2]  * a1 + back[2]  * (255 - a1)) >> 8),
                                 (BYTE)((front[1]  * a1 + back[1]  * (255 - a1)) >> 8),
                                 (BYTE)((front[0]  * a1 + back[0]  * (255 - a1)) >> 8));

    return color;
}

__m128i CalcAlphaSSE(const sf::Uint8 *front, const sf::Uint8 *back)
{
    __m128i fr;
    memcpy(&fr, front, sizeof(__m128i));

    __m128i bk;
    memcpy(&bk, back, sizeof(__m128i));

    __m128i _0 = _mm_set1_epi16(0);
    __m128i FR = (__m128i) _mm_movehl_ps((__m128) _0, (__m128) fr);
    __m128i BK = (__m128i) _mm_movehl_ps((__m128) _0, (__m128) bk);
    
    fr = _mm_cvtepu8_epi16(fr);
    bk = _mm_cvtepu8_epi16(bk);
    FR = _mm_cvtepu8_epi16(FR);
    BK = _mm_cvtepu8_epi16(BK);

    const __m128i movA_0 = _mm_set_epi8(0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF,
                                        0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF);
    const __m128i movA   = _mm_set_epi8(_z, 14, _z, 14, _z, 14, _z, 14, 
                                        _z, 6,  _z, 6,  _z, 6,  _z, 6);
    __m128i a = _mm_and_si128(_mm_shuffle_epi8(fr, movA), movA_0);
    __m128i A = _mm_and_si128(_mm_shuffle_epi8(FR, movA), movA_0);

    fr = _mm_mullo_epi16(fr, a);                                                //fr *= a
    FR = _mm_mullo_epi16(FR, A);

    __m128i _255 = _mm_set1_epi16(255);                                         //bk *= 255 - a
    bk = _mm_mullo_epi16(bk, _mm_sub_epi16(_255, a));
    BK = _mm_mullo_epi16(BK, _mm_sub_epi16(_255, A));

    __m128i sum = _mm_add_epi16(fr, bk);                                        //sum = fr + bk
    __m128i SUM = _mm_add_epi16(FR, BK);

    const __m128i movSum_0 = _mm_set_epi8(0,    0,    0,    0,    0,    0,    0,    0,
                                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);               //sum = sum >> 8
    const __m128i movSum   = _mm_set_epi8(_z, _z, _z, _z, _z, _z, _z, _z,
                                          15, 13, 11,  9,  7,  5,  3,  1);
    sum = _mm_and_si128(_mm_shuffle_epi8(sum, movSum), movSum_0);
    SUM = _mm_and_si128(_mm_shuffle_epi8(SUM, movSum), movSum_0);


    __m128i color = (__m128i) _mm_movelh_ps((__m128)sum, (__m128)SUM);

    return color;
}


int PrintCatPic()
{     
    sf::VertexArray pointmap(sf::Points, BK_WIDTH * BK_HEIGHT);

    sf::Image cat_pic; 
    cat_pic.loadFromFile("pics/AskhatCat.bmp");

    sf::Image tab_pic; 
    tab_pic.loadFromFile("pics/yasin.bmp");

    double start_time = clock();
    int err = CalcPointmapSSE(&pointmap, cat_pic.getPixelsPtr(), tab_pic.getPixelsPtr());
    ERR_CHCK(err, ERROR_CALC_MANDELBROT);

    double end_time = clock();

    double elapsed_time = (double)(end_time - start_time) /CLOCKS_PER_SEC;

    FILE *log_f = fopen("logs/AVXalphalog.txt", "a");
    ERR_CHCK(log_f == NULL, ERROR_FILE_OPENING);
    fprintf(log_f, "    done in %lf sec\n", elapsed_time);
    fclose(log_f);

    #ifndef LAB_MOD

    sf::RenderWindow window(sf::VideoMode(BK_WIDTH, BK_HEIGHT), "Alpha-Blending");
    window.setFramerateLimit(60);

    while (window.isOpen()) 
    {
        sf::Event event;
        while (window.pollEvent(event)) 
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(pointmap);
        window.display();
    }

    #endif

    return 0;
}