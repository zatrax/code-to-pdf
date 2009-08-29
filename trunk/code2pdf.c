
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "hpdf.h"

jmp_buf env;

#ifdef HPDF_DLL
void  __stdcall
#else
void
#endif
error_handler (HPDF_STATUS   error_no,
               HPDF_STATUS   detail_no,
               void         *user_data)
{
    printf ("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no,
                (HPDF_UINT)detail_no);
    longjmp(env, 1);
}

#define MAXSTRING 1000
#define LINES_PER_PAGE 64
#define MAX_FONT_SIZE_FOR_CODE 10
#ifndef min
#define min(a,b)  ((a) > (b) ? (b) : (a))
#endif
char fname[256] = "code.pdf"; /* default output file */
char page_title[MAXSTRING] = "Happy Code Reading";
const char code_font[MAXSTRING] = "Courier";
const char section_font[MAXSTRING] = "Courier-Bold";
const char default_font_name[MAXSTRING] = "Helvetica";
typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} RECT;

typedef struct {
    int x;
    int y;
} POINT;

typedef struct {
    int cx;
    int cy;
} SIZE;

enum RefType
{
    RT_NIL,
    RT_LEFT_BRACKET,
    RT_RIGHT_BRACKET,
    RT_LINE
};

typedef struct {
    char  line[MAXSTRING];
    int   line_num;
    int   split;
    int   ref;
    int   ref_data;
} LineInfo;

LineInfo lines[LINES_PER_PAGE];

#define GETWIDTH(r) ((r)->right - (r)->left)
#define GETHEIGHT(r) ((r)->bottom - (r)->top)

RECT page_margin_odd = {50, 80, 25, 50};
RECT page_margin_even = {25, 80, 50, 50};

RECT page_margin = {50, 50, 25, 25};
RECT page_padding = {10, 10, 10, 10};
SIZE page_size;

void PrintPageHeader(HPDF_Doc pdf, HPDF_Page page, const char *header)
{
    HPDF_Font header_font;
    HPDF_REAL tw;
    HPDF_REAL width;
    header_font = HPDF_GetFont (pdf, default_font_name, NULL);
    HPDF_Page_SetFontAndSize (page, header_font, 12);
    tw = HPDF_Page_TextWidth (page, header);
    HPDF_Page_BeginText (page);

    width = page_size.cx - page_margin.left - page_margin.right,
    HPDF_Page_TextOut (page, 
                       page_margin.left + (width - tw) / 2, 
                       page_size.cy - page_margin.top + 6, 
                       header);
    HPDF_Page_EndText (page);
}

void PrintPageContent(HPDF_Doc pdf, HPDF_Page page, LineInfo lines[], int n)
{
    HPDF_Font typewriter_font;
    HPDF_REAL tw;
    HPDF_REAL width;
    RECT    rcBox;
    int x, y;
    int font_size = 10;
    int i = 1;
    char buf[MAXSTRING];
    int actual_height = 0;
    int align;
    int line_num;

    if (n <= 0 || n > LINES_PER_PAGE)
    {
        printf("Error: %d lines\n", n);
        return;
    }

    rcBox.left = page_margin.left + page_padding.left;
    rcBox.right = page_size.cx - page_margin.right - page_padding.right;
    rcBox.top = page_size.cy - page_margin.top - page_padding.top;
    rcBox.bottom = page_margin.bottom + page_padding.bottom;

    font_size = (rcBox.top - rcBox.bottom) / n;
    font_size = min(MAX_FONT_SIZE_FOR_CODE, font_size);

    actual_height = LINES_PER_PAGE * font_size;
    align = (rcBox.top - rcBox.bottom - actual_height) / 2;
    rcBox.top -= align;
    rcBox.bottom += align;

    typewriter_font = HPDF_GetFont (pdf, "Courier", NULL);
    HPDF_Page_SetFontAndSize (page, typewriter_font, font_size);

    y = rcBox.top - font_size;
    x = rcBox.left;

    HPDF_Page_BeginText (page);
    HPDF_Page_MoveTextPos (page, x, y);

    line_num = lines[0].line_num;
    for (i = 0; i < LINES_PER_PAGE; i++, line_num++)
    {
        if (i < n)
        {
            sprintf(buf, "%04d %s", line_num, lines[i].line);
        }
        else
        {
            sprintf(buf, "%04d", line_num);
        }
        HPDF_Page_ShowText (page, buf);
        HPDF_Page_MoveTextPos (page, 0, -font_size);
    }

    HPDF_Page_EndText (page);
}

void PrintPageFooter(HPDF_Doc pdf, HPDF_Page page, const char *footer)
{
    HPDF_Font header_font;
    HPDF_REAL tw;
    HPDF_REAL width;
    header_font = HPDF_GetFont (pdf, default_font_name, NULL);
    HPDF_Page_SetFontAndSize (page, header_font, 12);
    tw = HPDF_Page_TextWidth (page, footer);
    HPDF_Page_BeginText (page);

    width = page_size.cx - page_margin.left - page_margin.right,
    HPDF_Page_TextOut (page, 
                       page_margin.left + (width - tw) / 2, 
                       page_margin.bottom - 12,
                       footer);
    HPDF_Page_EndText (page);
}

int main (int argc, char **argv)
{
    HPDF_Doc  pdf;
    HPDF_Page page;
    HPDF_Font def_font;
    HPDF_REAL tw;
    HPDF_REAL height;
    HPDF_REAL width;
    HPDF_UINT i;
    int page_num = 1;
    int done = 0;
    char page_number[100];
    int line_num = 1;

    pdf = HPDF_New (error_handler, NULL);
    if (!pdf) {
        printf ("error: cannot create PdfDoc object\n");
        return 1;
    }

    if (setjmp(env)) {
        HPDF_Free (pdf);
        return 1;
    }

    while (!done)
    {
        int n = 0;
        char buf[1000];
        char *p;

        while ((p = fgets(buf, 999, stdin)) && n < LINES_PER_PAGE)
        {
            if (buf[0] == '@')
            {
                if (strncmp(buf, "@title ", 7)==0)
                {
                    strcpy(page_title, buf+7);
                }
                else if (strncmp(buf, "@newpage", 8)==0)
                {
                    if (n != 0 && n != LINES_PER_PAGE-1)
                    {
                        line_num += LINES_PER_PAGE;
                        line_num -= line_num % LINES_PER_PAGE;
                        line_num ++;
                        break;
                    }
                }
                continue;
            }
            lines[n].line_num = line_num++;
            strcpy(lines[n].line, buf);
            n++;
        }

        if (p == NULL)
        {
            // We have seen the end of input
            done = 1;
        }

        if (n == 0)
        {
            // No lines read, job done
            break;
        }

        /* Add a new page object. */
        page = HPDF_AddPage (pdf);
        HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT); 

        height = HPDF_Page_GetHeight (page);
        width = HPDF_Page_GetWidth (page);

        page_margin = (page_num % 2 == 0 ? page_margin_even : page_margin_odd);

        page_size.cx = (int)width;
        page_size.cy = (int)height;

        def_font = HPDF_GetFont (pdf, default_font_name, NULL);
        HPDF_Page_SetFontAndSize (page, def_font, 16);

        PrintPageHeader(pdf, page, page_title);
        PrintPageContent(pdf, page, lines, n);
        sprintf(page_number, "%d", page_num);
        PrintPageFooter(pdf, page, page_number);
        page_num++;
    }
    HPDF_SaveToFile (pdf, fname);

    /* clean up */
    HPDF_Free (pdf);

    return 0;
}

