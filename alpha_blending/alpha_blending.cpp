
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

//#include <BaseTsd.h>
#include <WinDef.h>
#include <wingdi.h>

#include <emmintrin.h>
#include <immintrin.h>

#define LAB_MOD
#undef  LAB_MOD


int PrintCatPic();

void *GetRGBbufPtr(const char *file_name, int *rgbs_amnt, int *buf_width, int *buf_height);
int   CalcPointmap    (sf::VertexArray *pointmap);

sf::Color CalcAlpha    (RGBQUAD *front, RGBTRIPLE *back);
sf::Color CalcAlphaAVX2(RGBQUAD *front, RGBTRIPLE *back);


int main(int argc, char *argv[])
{

    int err = PrintCatPic();
    ERR_CHCK(err, ERROR_PRINT_MANDELBROT);

    return SUCCESS;
}


int PrintPic(sf::VertexArray *pointmap)
{
    FILE *log_f = fopen("alphalog.txt", "w");
    ERR_CHCK(log_f == NULL, ERROR_FILE_OPENING);
    //fprintf(log_f, "File opened dx = %.4f, dy = %.4f\n", dx, dy);
    FILE *cat_f = fopen("pics/AskhatCat.bmp", "rb");
    FILE_ERR_CHCK(log_f == NULL, ERROR_FILE_OPENING, log_f);

    BITMAPFILEHEADER bfh;
    fread(&bfh, sizeof(char), sizeof(BITMAPFILEHEADER), cat_f);


    BITMAPINFOHEADER bih;
    fread(&bih, sizeof(char), sizeof(BITMAPINFOHEADER), cat_f);

    int       f_size = (int)bfh.bfSize;
    int   headr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    int rgb_buf_size = f_size - headr_size;
    

    long width  = bih.biWidth;
    long height = bih.biHeight;
    int  bit_c  = bih.biBitCount / 8;

    char *cat_buf = (char *) calloc (rgb_buf_size, sizeof(char));
    fread(cat_buf, sizeof(char), rgb_buf_size, cat_f);

    int err = fclose(cat_f);
    FILE_ERR_CHCK(err == EOF, ERROR_FILE_CLOSING, log_f);

    int rgbq_amnt = rgb_buf_size / bit_c;

    fprintf(log_f,  "f_size        = %d\n"
                    "headr_size    = %d\n"
                    "rgb_buf_size  = %d\n"
                    "rgbs_amount   = %d\n\n"
                    "width         = %d\n"
                    "height        = %d\n"
                    "bitCount      = %d\n"
                    "XPelsPerMeter = %d\n"
                    "YPelsPerMeter = %d\n",
            f_size, headr_size, rgb_buf_size, rgbq_amnt, width, height, bih.biBitCount, bih.biXPelsPerMeter, bih.biYPelsPerMeter);

    if (bit_c == 4)
    {
        RGBQUAD *front = (RGBQUAD *)cat_buf;

        for (int i = 0; i < rgbq_amnt; i++)
        {
            //int offset = (i % 235) + (167 - (i / 235)) * 800;
            fprintf(log_f, "[%d]: x = %d, y = %d (rgbr = %d, %d, %d, %d)\n", 
                    i, i % width, height - (i / width), front[i].rgbRed, front[i].rgbGreen, front[i].rgbBlue, front[i].rgbReserved);

            (*pointmap)[i].position = sf::Vector2f(i % width, height - (i / width));
            (*pointmap)[i].color    = sf::Color{front[i].rgbRed, front[i].rgbGreen, front[i].rgbBlue, front[i].rgbReserved};
        }
    }

    else if (bit_c == 3)
    {
        RGBTRIPLE *front = (RGBTRIPLE *)(cat_buf + 28 * 3);

        for (int i = 0; i < (rgbq_amnt - 28); i++)
        {
            //int offset = (i % 235) + (167 - (i / 235)) * 800;
            fprintf(log_f, "[%d]: x = %d, y = %d (rgb = %d, %d, %d)\n", 
                    i, i % width, height - (i / width), front[i].rgbtRed, front[i].rgbtGreen, front[i].rgbtBlue);

            (*pointmap)[i].position = sf::Vector2f(i % width, height - (i / width));
            (*pointmap)[i].color    = sf::Color{front[i].rgbtRed, front[i].rgbtGreen, front[i].rgbtBlue};
        }
    }
    free(cat_buf);
    err = fclose(log_f);
    ERR_CHCK(err == EOF, ERROR_FILE_CLOSING);

    return SUCCESS;
}



int CalcPointmap(sf::VertexArray *pointmap)
{
    int fr_rgbs_amnt = 0;
    int bk_rgbs_amnt = 0;

    int fr_width  = 0;
    int bk_width  = 0;

    int fr_height = 0;
    int bk_height = 0;

    RGBQUAD   *fr_buf = (RGBQUAD *)GetRGBbufPtr("pics/AskhatCat.bmp", &fr_rgbs_amnt, &fr_width, &fr_height);
    ERR_CHCK(  fr_buf == NULL, ERROR_GET_RGB_BUF);
    fr_buf = (RGBQUAD *)((char *)fr_buf + 4);

    RGBTRIPLE *bk_buf = (RGBTRIPLE *)GetRGBbufPtr("pics/Table.bmp",   &bk_rgbs_amnt, &bk_width, &bk_height);
    ERR_CHCK(  bk_buf == NULL, ERROR_GET_RGB_BUF);
    bk_buf = (RGBTRIPLE *)((char *)bk_buf);
    //bk_rgbs_amnt -= 28;

    FILE *log_f = fopen("alphalog.txt", "a");
    ERR_CHCK(log_f == NULL, ERROR_FILE_OPENING);

    fprintf(log_f, "BEFORE POINTMAPPING: bk_width = %d, bk_height = %d, bk_rgbs_amnt = %d\n",
                                         bk_width,      bk_height,      bk_rgbs_amnt);

    
    int step = 4;
    for (int bk_i = 0, fr_i = 0; bk_i < bk_rgbs_amnt; bk_i += step)
    {
        //int offset = (i % 235) + (167 - (i / 235)) * 800;
        //fprintf(log_f, "[%d]: x = %d, y = %d (rgbr = %d, %d, %d, %d)\n", 
          //      i, i % cat_width, cat_height - (i / cat_width), front[i].rgbRed, front[i].rgbGreen, front[i].rgbBlue, front[i].rgbReserved);

        float x = bk_i % bk_width;
        float y = bk_height - (bk_i / bk_width);
    
        (*pointmap)[bk_i].position = sf::Vector2f(x, y);
        if ((x >= ((bk_width - fr_width) >> 1)) && (x < ((bk_width + fr_width) >> 1)) && (y < 350) && y > (350 - fr_height))
        {
            fprintf(log_f, "[%d(%d)]: x = %.3f, y = %.3f (rgbr = %d, %d, %d, %d)\n", bk_i, fr_i, x, y, 
                            fr_buf[fr_i].rgbRed, fr_buf[fr_i].rgbGreen, fr_buf[fr_i].rgbBlue, fr_buf[fr_i].rgbReserved);
            
            (*pointmap)[bk_i].color = CalcAlphaAVX2(&fr_buf[fr_i], &bk_buf[bk_i]);
            fr_i += step;
        }

        else
            (*pointmap)[bk_i].color = sf::Color{bk_buf[bk_i].rgbtRed, bk_buf[bk_i].rgbtGreen, bk_buf[bk_i].rgbtBlue};
    }
    
    free(bk_buf);
    free(fr_buf);
    

    int err = fclose(log_f);
    ERR_CHCK(err == EOF, ERROR_FILE_CLOSING);

    return SUCCESS;
}


void *GetRGBbufPtr(const char *file_name, int *rgbs_amnt, int *buf_width, int *buf_height)
{
    //fprintf(log_f, "File opened dx = %.4f, dy = %.4f\n", dx, dy);
    FILE *file_ptr = fopen(file_name, "rb");
    ERR_CHCK(file_ptr == NULL, NULL);

    BITMAPFILEHEADER bfh;
    fread(&bfh, sizeof(char), sizeof(BITMAPFILEHEADER), file_ptr);

    BITMAPINFOHEADER bih;
    fread(&bih, sizeof(char), sizeof(BITMAPINFOHEADER), file_ptr);

    int       f_size = (int)bfh.bfSize;
    int   headr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    int rgb_buf_size = f_size - headr_size;
    

    long width  = bih.biWidth;
    long height = bih.biHeight;
    int  byte_c = bih.biBitCount / 8;

    char *buf = (char *) calloc (rgb_buf_size, sizeof(char));
    fread(buf, sizeof(char), rgb_buf_size, file_ptr);

    int err = fclose(file_ptr);
    ERR_CHCK(err == EOF, NULL);

    int rgb_strct_amnt = rgb_buf_size / byte_c;

    FILE *log_f = fopen("alphalog.txt", "a");
    ERR_CHCK(log_f == NULL, NULL);
    fprintf(log_f,  "%s (readed):\n"
                    "   f_size        = %d\n"
                    "   headr_size    = %d\n"
                    "   rgb_buf_size  = %d\n"
                    "   rgbq_amount   = %d\n\n"
                    "   width         = %d\n"
                    "   height        = %d\n"
                    "   bitCount      = %d\n"
                    "   XPelsPerMeter = %d\n"
                    "   YPelsPerMeter = %d\n\n",
            file_name, f_size, headr_size, rgb_buf_size, rgb_strct_amnt, 
            width, height, bih.biBitCount, bih.biXPelsPerMeter, bih.biYPelsPerMeter);

    err = fclose(log_f);
    ERR_CHCK(err == EOF, NULL);

    *rgbs_amnt  = rgb_strct_amnt;
    *buf_width  = width;
    *buf_height = height;

    return buf;
}


sf::Color CalcAlpha(RGBQUAD *front, RGBTRIPLE *back)
{
    BYTE a = front->rgbReserved;
    sf::Color color =  sf::Color{(BYTE)((front->rgbRed   * a + back->rgbtRed   * (255 - a)) >> 8),
                                 (BYTE)((front->rgbBlue  * a + back->rgbtBlue  * (255 - a)) >> 8),
                                 (BYTE)((front->rgbGreen * a + back->rgbtGreen * (255 - a)) >> 8)};

    return color;
}


sf::Color CalcAlphaAVX2(RGBQUAD *front, RGBTRIPLE *back)
{
    //BYTE a = front[i].rgbReserved;

    // __m256i fr = _mm256_load_si256((__m256i *) &(front[fr_i]));
    // __m256i bk = _mm256_load_si256((__m256i *) &(back [bk_i]));

    // sf::Color color = sf::Color::Majenta;
    // return color;
}


int PrintCatPic()
{     
    sf::VertexArray pointmap(sf::Points, 800 * 600);
    int err = CalcPointmap(&pointmap);
    ERR_CHCK(err, ERROR_CALC_MANDELBROT);

    #ifndef LAB_MOD

    sf::RenderWindow window(sf::VideoMode(800, 600), "Test Window");
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