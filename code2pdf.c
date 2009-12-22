/*******************************************************************************

                    CODE2PDF - Code Printing Helper

                             Chaoji Li
                            Oct 3, 2009

For me, good codes are those when you print them out you think they worth the 
paper and ink. Our ancestor has used much of paper in their programming,
like punch card, paper tape. It is quite a waste. They even use paper 
for terminal listing. "dir/ls". Think of that.

Find some well written code and make comments on them. 
There are two options:
   - 80x64 on A4 paper portrait. This is very common in code of old era.
     slim and well planned.
   - 120x50 on A4 paper landscape. Code get fatter nowadays. Especially for c# or java

Stamp random quotations on the header of a sheet. Quotes from old programmers.
Or one line joke. 

Printing on paper enables you to make comments by pen.  Code reading is a very
personal thing.  Use pen make me feel good. You can put some illustrations 
inside the paper to describe something behind the codes. Put line profiling 
results or memory usages or sample output or callstack to help understand
the behaviors.
 
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "hpdf.h"

#define MAXSTRING 1000

#define CODE_FONT_SIZE 10
#define EVENPAGE(n) (!((n) & 1))

#ifndef min
#define min(a,b)  ((a) > (b) ? (b) : (a))
#endif

typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} RECT;

typedef struct {
    int cx;
    int cy;
} SIZE;

typedef struct {
    char  line[MAXSTRING];
    int   line_num;
} LINE_INFO;

char fname[256] = "code.pdf"; /* default output file */
char page_title[MAXSTRING] = "Happy Code Reading";
const char code_font[MAXSTRING] = "Courier";
const char section_font[MAXSTRING] = "Courier-Bold";
const char default_font_name[MAXSTRING] = "Helvetica";
LINE_INFO lines[100];
int  no_line_number = 0;
int  use_landscape = 0;
RECT page_margin_odd = {50, 80, 25, 50};
RECT page_margin_even = {25, 80, 50, 50};
RECT page_margin_odd_landscape = {50, 50, 25, 30};
RECT page_margin_even_landscape = {25, 50, 50, 30};
RECT page_margin_kindle = {30, 80, 30, 50};
RECT page_margin = {50, 50, 25, 25};
RECT page_padding = {10, 10, 10, 10};
SIZE page_size;
jmp_buf env;
int  LINES_PER_PAGE = 64;

void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data)
{
    printf ("Internal error: error_no=%04X, detail_no=%u\n",
            (HPDF_UINT)error_no,(HPDF_UINT)detail_no);
    printf("Please make sure file %s is writable. If it is opened by\n"
           "other applications, please close it and try again.\n", fname);
    longjmp(env, 1);
}

void print_page_header(HPDF_Doc pdf, HPDF_Page page, const char *header)
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

void print_page_content(HPDF_Doc pdf, HPDF_Page page, LINE_INFO lines[], int n)
{
    HPDF_Font typewriter_font;
    RECT rcBox;
    int i, x, y;
    int font_size = CODE_FONT_SIZE;
    char buf[MAXSTRING];
    int content_height;
    int align;
    int line_num;

    /* rcBox is the bounding box of content area
     * get rid of the margin and padding */
    rcBox.left = page_margin.left + page_padding.left;
    rcBox.right = page_size.cx - page_margin.right - page_padding.right;
    rcBox.top = page_size.cy - page_margin.top - page_padding.top;
    rcBox.bottom = page_margin.bottom + page_padding.bottom;


    /* Try to position rcBox at the center of content area
     * If align turns out to be negative, then the rcBox is enlarged */
    content_height = LINES_PER_PAGE * font_size;
    align = (rcBox.top - rcBox.bottom - content_height) / 2;
    rcBox.top -= align;
    rcBox.bottom += align;
    
    typewriter_font = HPDF_GetFont (pdf, "Courier", NULL);
    HPDF_Page_SetFontAndSize (page, typewriter_font, font_size);
    HPDF_Page_BeginText(page);
    x = rcBox.left;
    y = rcBox.top - font_size;
    HPDF_Page_MoveTextPos(page, x, y);
    line_num = lines[0].line_num;
    for (i = 0; i < LINES_PER_PAGE; i++, line_num++)
    {
        if (no_line_number)
        {
            sprintf(buf, "  %s", i < n ? lines[i].line : "");
        }
        else
        {
            sprintf(buf, "%04d %s", line_num, i < n ? lines[i].line : "");
        }
        HPDF_Page_ShowText(page, buf);
        HPDF_Page_MoveTextPos(page, 0, -font_size);
    }
    HPDF_Page_EndText (page);
}

void print_page_footer(HPDF_Doc pdf, HPDF_Page page, const char *footer)
{
    HPDF_Font header_font;
    HPDF_REAL tw;
    HPDF_REAL width;
    
    header_font = HPDF_GetFont(pdf, default_font_name, NULL);
    HPDF_Page_SetFontAndSize(page, header_font, 12);
    tw = HPDF_Page_TextWidth(page, footer);
    width = page_size.cx - page_margin.left - page_margin.right,
    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page, 
                      page_margin.left + (width - tw) / 2, 
                      page_margin.bottom - 12,
                      footer);
    HPDF_Page_EndText(page);
}

void usage()
{
    printf("Usage: code2pdf <filename>\n"
           "  -            Accepts text from stdin.\n"
           "  -t  <title>  header string\n"
           "  -o  <output> output file name, default code.pdf\n"
           "  -l           landscale, 120x50\n"
           "  -k           for kindle\n"
           "  -n           no line number\n"
           "  -h           help\n");
    exit(-1);
}

int main (int argc, char **argv)
{
    HPDF_Doc  pdf;
    HPDF_Page page;
    HPDF_Font def_font;
    HPDF_REAL height;
    HPDF_REAL width;
    HPDF_UINT i;
    int page_num = 1;
    int done = 0;
    char page_number[MAXSTRING];
    int line_num = 1;
    FILE *input_file = NULL;
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-t") == 0)
        {
            strcpy(page_title, argv[i+1]);
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            strcpy(fname, argv[i+1]);
        }
        else if (strcmp(argv[i], "-k") == 0)
        {
            page_margin_odd = page_margin_kindle;
            page_margin_even = page_margin_kindle;
        }
        else if (strcmp(argv[i], "-n") == 0)
        {
            no_line_number = 1;
        }
        else if (strcmp(argv[i], "-h")==0)
        {
            usage();
        }
        else if (strcmp(argv[i], "-l") == 0)
        {
            use_landscape = 1;
            page_margin_odd = page_margin_odd_landscape;
            page_margin_even = page_margin_even_landscape;
            LINES_PER_PAGE=50;
        }
        else if (strcmp(argv[i], "-")==0)
        {
            input_file = stdin;
        }
        else
        {
            input_file = fopen(argv[i], "r");
        }
    }

    if (input_file == NULL)
    {
        printf("error: no input specified\n");
        usage();
    }

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

        while ((p = fgets(buf, 999, input_file)) && n < LINES_PER_PAGE)
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

        /* If no more input in future, this is the last page,
         * flag as done.
         */
        if (p == NULL)
            done = 1;

        /* If no lines are read, we are done. */
        if (n == 0)
            break;

        /* Add a new page object. */
        page = HPDF_AddPage (pdf);
        HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_A4, use_landscape?HPDF_PAGE_LANDSCAPE:HPDF_PAGE_PORTRAIT); 
        HPDF_Page_SetRGBFill(page, 0, 0, 0);
        
        height = HPDF_Page_GetHeight (page);
        width = HPDF_Page_GetWidth (page);

        page_margin = EVENPAGE(page_num) ? page_margin_even : page_margin_odd;
        page_size.cx = (int)width;
        page_size.cy = (int)height;

        def_font = HPDF_GetFont (pdf, default_font_name, NULL);
        HPDF_Page_SetFontAndSize (page, def_font, 16);
        print_page_header(pdf, page, page_title);
        print_page_content(pdf, page, lines, n);
        sprintf(page_number, "%d", page_num);
        print_page_footer(pdf, page, page_number);
        page_num++;
    }
    if (page_num > 1)
    {
        HPDF_SaveToFile (pdf, fname);
        printf("Output written to %s\n", fname);
    }
    else
    {
        printf("No output\n");
    }
    
    if (input_file != stdin && input_file != NULL)
        fclose(input_file);

    /* clean up */
    HPDF_Free (pdf);
    return 0;
}
/************************************* END ************************************/
