
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>

#include "common.h"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <sstream>
#include <iostream>


//#include <emmintrin.h>
#include <immintrin.h>

#define LAB_MOD
#undef  LAB_MOD


int PrintMandelbrot();
int CalcMandelbrot(sf::VertexArray *pointmap, float x_off, float y_off, float ppe);
int CalcMandelbrotAVX2(sf::VertexArray *pointmap, float x_off, float y_off);

int main(int argc, char *argv[])
{

    int err = PrintMandelbrot();
    ERR_CHCK(err, ERROR_PRINT_MANDELBROT);

    return SUCCESS;
}


int CalcMandelbrot(sf::VertexArray *pointmap, float x_off, float y_off, float ppe)
{
    float dx = 1 / ppe;
    float dy = 1 / ppe;

    x_off /= ppe;
    y_off /= ppe;


    int xMax  = 1000;
    int yMax  = 1000;
    int nMax  = 256;

    float rMax2 = 100;

    float x0 = -2 - x_off;
    float y0 = 2  + y_off;

    float x = x0;
    float y = y0;

    sf::Uint8 N = 0;
    FILE *log_f = fopen("logs/log.txt", "w");
    //fprintf(log_f, "File opened dx = %.4f, dy = %.4f\n", dx, dy);
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

            int offset = ((x0 + 2 + x_off) + (2 + y_off - y0) * 1000) * ppe;
            //fprintf(log_f, "n[%d] = %d, offset = %d (x0 = %.3f, y0 = %.3f)\n", xi, n, offset, x0, y0);
            //(*pointmap)[offset].position = sf::Vector2f((x0 + 2 + x_off) * ppe, (2 + y_off - y0) * ppe);
            //(*pointmap)[offset].color = sf::Color{(N % 4) * 64, N, N};

            x0 += dx;
        }

        x0 = -2 - x_off;
        y0 -= dy;
    }
    fclose(log_f);

    return SUCCESS;
}


int CalcMandelbrotAVX2(sf::VertexArray *pointmap, float x_off, float y_off, float ppe)
{
    __m256 dx = _mm256_set1_ps(1 / ppe);
    __m256 dy = _mm256_set1_ps(1 / ppe);

    x_off /= ppe;
    y_off /= ppe;

    int xMax  = 1000 / 8;
    int yMax  = 1000;
    int nMax  = 256;

    __m256 rMax2 = _mm256_set1_ps(100);

    __m256 _01234567 = _mm256_set_ps(0, 1, 2, 3, 4, 5, 6, 7);
    __m256 dx_01234567 = _mm256_mul_ps(dx, _01234567);
    __m256 dx_8        = _mm256_mul_ps(dx, _mm256_set1_ps(8));

    __m256 x0 = _mm256_set1_ps(-2 - x_off);
           x0 = _mm256_add_ps(x0, dx_01234567);
    __m256 y0 = _mm256_set1_ps(2 + y_off);

    FILE *log_f = fopen("logs/AVXlog.txt", "w");
    //fprintf(log_f, "File opened dx = %.4f, dy = %.4f\n", dx, dy);
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

                //cntr = _mm256_add_ps(cntr, _mm256_and_ps(index, mask));
                cntr = _mm256_add_ps(cntr, _mm256_and_ps(index, mask));

                if (_mm256_movemask_ps(mask) == 0)
                    break;

                
            }


            #ifndef LAB_MOD


            for (int i = 0; i < 8; i++)
            {
                int offset = ((x0[7 - i] + 2 + x_off) + (2 + y_off - y0[7 - i]) * 1000) * ppe;
                //fprintf(log_f, "n[%d] = %f, offset = %d (x0 = %.3f, y0 = %.3f)\n", xi * 8 + i, n_arr[7 - i], offset, x0_arr[7 - i], y0_arr[7 - i]);
                (*pointmap)[offset].position = sf::Vector2f((x0[7 - i] + 2 + x_off) * ppe, (2 + y_off - y0[7 - i]) * ppe);
                (*pointmap)[offset].color = sf::Color{((sf::Uint8)cntr[7 - i] % 4) * 64, (sf::Uint8)cntr[7 - i], (sf::Uint8)cntr[7 - i]};

            }
            #endif

            x0 = _mm256_add_ps(x0, dx_8);
        }

        x0 = _mm256_set1_ps(-2 - x_off);
        x0 = _mm256_add_ps(x0, dx_01234567);
        y0 = _mm256_sub_ps(y0, dy);
    }
    fclose(log_f);

    return SUCCESS;
}


sf::Time thetime;
sf::Clock elapsed;
std::string GetFrameRate()
{    
    static int frameCounter = 0;
    static int fps = 0;
    frameCounter++;
    thetime = elapsed.getElapsedTime();
    if (thetime.asMilliseconds() > 999)
    {
        fps = frameCounter;
        frameCounter = 0;
        elapsed.restart();
    }

    std::stringstream ss;
    ss << fps;

    return ss.str();
}

int PrintMandelbrot()
{
    float x_off = 0;
    float y_off = 0;
    float ppe   = 1000/4;
     
    sf::VertexArray pointmap(sf::Points, 1000 * 1000);
    int err = CalcMandelbrotAVX2(&pointmap, x_off, y_off, ppe);
    ERR_CHCK(err, ERROR_CALC_MANDELBROT);

    #ifndef LAB_MOD

    sf::RenderWindow window(sf::VideoMode(1000, 1000), "Mandelbrot.exe");
    window.setFramerateLimit(60);
    sf::Font font;
    font.loadFromFile("font/pixeleum-48.ttf");
    sf::Text FPS("0", font, 25);
    // for (int t = 0; t < 1000 * 1000; t++)
    // {
    //     pointmap[t].color = sf::Color{255, 0, 255};
    //     pointmap[t].position = sf::Vector2f(700, t % 1000);
    // }
    
    bool calc_f = false;
    while (window.isOpen()) 
    {
        sf::Event event;
        while (window.pollEvent(event)) 
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        if      (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  { x_off += 25;
                                                                    calc_f = true; }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { x_off -= 25;
                                                                    calc_f = true; }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    { y_off += 25;
                                                                    calc_f = true; }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  { y_off -= 25;
                                                                    calc_f = true; }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::P))     { ppe *= 2;
                                                                    x_off -= (ppe * 2 - 500) + x_off;
                                                                    y_off -= (ppe * 2 - 500) + y_off;
                                                                    calc_f = true; }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::O))     { ppe /= 2;
                                                                    x_off += (500 - ppe * 2) - x_off;
                                                                    y_off += (500 - ppe * 2) - y_off;
                                                                    calc_f = true; }
        if (calc_f)
        {
            err = CalcMandelbrotAVX2(&pointmap, x_off, y_off, ppe);
            ERR_CHCK(err, ERROR_CALC_MANDELBROT);
            calc_f = false;
        }

        window.clear();
        window.draw(pointmap);
        FPS.setString(GetFrameRate());
        window.draw(FPS);
        window.display();
    }

    #endif

    return 0;
}



void add_farrs(float *farr1, float *farr2) { for(int i = 0; i < 4; i++) farr1[i] += farr2[i];}
void sub_farrs(float *farr1, float *farr2) { for(int i = 0; i < 4; i++) farr1[i] -= farr2[i];}
void mul_farrs(float *farr1, float *farr2) { for(int i = 0; i < 4; i++) farr1[i] *= farr2[i];}
void cpy_farrs(float *farr1, float *farr2) { for(int i = 0; i < 4; i++) farr1[i]  = farr2[i];}


int PrintMandelbrotASSE(void)
{
    float dx = 1 / (1000 / 4);
    float dy = 1 / (1000 / 4);

    int xMax = 1000 / 4;
    int yMax = 1000 / 4;
    int nMax = 255;
    int rMax = 10;

    int r2_4[] = {100, 100, 100, 100};
    int n_4[]  = {0, 0, 0, 0};
    int i_4[]  = {1, 1, 1, 1};

    float x0_4[] = {-2, -2 + dx, -2 + dx + dx, -2 + dx * 3};
    float dx_4[] = {dx, dx, dx, dx};

    float y0_4[] = {2, 2, 2, 2};
    float dy_4[] = {dy, dy, dy, dy};
    
    float x_4[]  = {0, 0, 0, 0};
    float y_4[]  = {0, 0, 0, 0};
    float xy_4[] = {0, 0, 0, 0};
    float x2_4[]  = {0, 0, 0, 0};
    float y2_4[]  = {0, 0, 0, 0};

    for  (int yi = 0; yi < yMax; yi++)
    {
        cpy_farrs(y_4, y0_4);


        for (int xi = 0; xi < xMax; xi++)
        {
            cpy_farrs(x_4, x0_4);

            for (int n = 0; n < nMax; n++)
            {
                cpy_farrs(xy_4, x_4); 
                mul_farrs(xy_4, y_4);
                add_farrs(xy_4, xy_4);
                add_farrs(xy_4, x0_4);

                mul_farrs(x_4, x_4);
                mul_farrs(y_4, y_4);
                sub_farrs(x_4, y_4);
                add_farrs(x_4, y0_4);

                cpy_farrs(y_4, xy_4);


                cpy_farrs(x2_4, x_4);
                cpy_farrs(y2_4, y_4);
                mul_farrs(x2_4, x2_4);
                mul_farrs(y2_4, y2_4);
                add_farrs(x2_4, y2_4);

                for (int i = 0; i < 4; i++)
                {
                
                }
            }

            

            add_farrs(x0_4, dx_4);
        }

        add_farrs(y0_4, dy_4);
    }

    return SUCCESS;
} 