#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define WRAP  80
#define ISSPACE(c) ((c)==' '||(c)=='\t')
#define ISWORD(c)  (isalpha(c) || (c)=='_' || isdigit(c))
#define ISGOOD(c)  ((c) == ',' || (c) == '(' || (c) == ')')

char* remove_spaces(char *p)
{
    char *q, *t;
    char *old;
    int cnt;

    old = p;

    /* Remove trailing spaces */
    while (*p)
        p++;
    while (p > old && isspace(*--p))
        ;
    *++p = 0;

    p = old;
    cnt = strlen(p) - WRAP;
    if (cnt <= 0)
        return old;

    /* Collapes middle spaces to 1  */

    while (ISSPACE(*p))
        p++;
    if (*p == '\0')
        return old;

    q = p;
    t = p;
    while (*q != '\0')
    {
        if (ISSPACE(*q))
        {
            while (ISSPACE(*(q+1)))
            {
                cnt--;
                q++;
            }
            *p = ' ';
        }
        else
        {
            *p = *q;
        }
        p++;
        q++;
    }
    *p = 0;
    if (cnt <= 0) return old;

    /* Remove uncessary spaces */

    p = t;
    q = p;
    while (*q != '\0')
    {
        if (ISSPACE(*q))
        {
            if (cnt > 0 && (
                    ISWORD(*(q-1)) != ISWORD(*(q+1)) ||
                    ISGOOD(*(q-1)) ||
                    ISGOOD(*(q+1))
                ))
            {
                cnt--;
            }
            else
            {
                *p = *q;
                p++;
            }
        }
        else
        {
            *p = *q;
            p++;
        }
        q++;
    }
    *p = 0;

    if (cnt <= 0)
        return old;

    /* We have to try to remove leading spaces */
    {
        int leading_spaces = 0;
        p = old;
        while (isspace(*p))
        {
            leading_spaces++;
            p++;
        }

        /* Even remove the leading spaces we still need to split
        *  Don't bother do it
        */
        if (cnt > leading_spaces)
            return old;
        else
            return old + cnt;
    }
}


int split_code_line(char *buf, char *dest)
{
    int split = 0;
    char *p;
    
    p = remove_spaces(buf);
    do 
    {
        int n = WRAP;
        while (*p != '\0' && n > 0)
        {
            if (*p != '\n' && *p != '\r') *dest++ = *p;
            p++;
            n--;
        }

        /* we only break on word boundary */
        if (*p != 0 && ISWORD(*p))
        {
            /*FIXME what if this line has a super long word */
            while (ISWORD(*(p-1)))
            {
                p--;
                dest--;
            }
        }
        
        *dest++ = '\n';
        if (*p && strlen(p) < WRAP)
        {
            int padding = 0;
            padding = WRAP-strlen(p);
            while (padding--)
            {
                *dest++ = ' ';
            }
        }

        split++;
    } while (*p);
    *dest++ = '\0';
    return split;
}



typedef struct
{
    char *buf;
} LineInfo;

typedef struct
{
    int beg;
    int end;
} Range;

LineInfo lines[10000];


int isregionstart(char *p)
{
    char *old;
    old = p;
    while (*p && isspace(*p))
    {
        p++;
    }
    if (strncmp(p, "//[", 3) != 0)
        return 0;
    if (strncmp(p, "//[End", 6) == 0)
        return 0;
    while (*p != 0)
        p++;
    while (p > old && isspace(*(--p)))
        ;
    if (*p == ']')
        return 1;
    else
        return 0;
}

int isregionend(char *p)
{
    char *old;
    old = p;
    while (*p && isspace(*p))
    {
        p++;
    }

    if (strncmp(p, "//[End", 6) != 0)
        return 0;

    while (*p != 0)
        p++;

    while (p > old && isspace(*(--p)))
        ;

    if (*p == ']')
        return 1;
    else
        return 0;
}

void print(char *s)
{
    if (strlen(s) > WRAP)
    {
        char dest[1000];
        split_code_line(s, dest);
        printf("%s", dest);
    }
    else
    {
        printf("%s", s);
    }
}

int get_leading_spaces(char *p)
{
    int nleadingspaces = 0;

    while (ISSPACE(*p))
    {
        nleadingspaces++;
        p++;
    }
    return nleadingspaces;
}

char *skip_leading_spaces(char *p)
{
    while (ISSPACE(*p))
        p++;
    return p;
}

char *skip_n_leading_spaces(char*p, int ls)
{
    while (ISSPACE(*p) && ls > 0)
        p++, ls--;
    return p;
}


void PrintRange(Range r, int level)
{
    int rcnt = 0;
    Range rstack[100];
    int i = 0;

    char buf[1000];


    int skip = 0;
    char *pbeg, *pend;
    int nleadingspaces;
    int j;
    if (level > 0)
    {
        pbeg = skip_leading_spaces(lines[ r.beg ].buf);
        pend = skip_leading_spaces(lines[ r.end ].buf);
        nleadingspaces = get_leading_spaces(lines[ r.beg ].buf);
        print(pbeg);
        print("{\n");
        r.beg++;
        r.end--;
    }
    
    for (i = r.beg; i <= r.end; i++)
    {
        if (!skip && isregionstart(lines[i].buf))
        {
            print(lines[i].buf);
            rstack[rcnt].beg = i;
            skip = 1;
        }
        else if (isregionend(lines[i].buf) &&
                 get_leading_spaces(lines[i].buf) == get_leading_spaces(lines[rstack[rcnt].beg].buf))
        {
            rstack[rcnt].end = i;
            rcnt++;
            skip = 0;
        }
        else if (level == 0 && lines[i].buf[0] == '}')
        {
            print(lines[i].buf);
            #if 0
           print("/* ----------------------------------- */\n\n");
           #endif
            for (j = 0; j < rcnt; j++)
                PrintRange(rstack[j], level + 1);

            rcnt = 0;
        }
        else if (!skip)
        {
            if (level > 0)
            {
                sprintf(buf, "    %s", skip_n_leading_spaces(lines[i].buf, nleadingspaces));
                print(buf);
            }
            else
                print(lines[i].buf);
        }
    }

    if (level > 0)
    {
        sprintf(buf, "} %s", pend);
        print(buf);
//        print("/* ----------------------------------- */\n\n");
    }

    for (j = 0; j < rcnt; j++)
        PrintRange(rstack[j], level + 1);
}

int tabwidth = 8;

int main(int argc, char *argv[])
{
    char buf[1000];
    int i, n = 0;
    Range r;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-t") == 0)
        {
            i++;
            tabwidth = atoi(argv[i]);
        }
    }
    
    while (fgets(buf, 999, stdin))
    {
        int j = 0;
        lines[n].buf = (char*)malloc(strlen(buf) + 100);
        i = 0;
        do 
        {
            if (buf[i] == '\t')
            {
                int k = 0;
                for (; k < tabwidth; k++)
                    lines[n].buf[j++] = ' ';
            }
            else
            {
                lines[n].buf[j++] = buf[i];
            }
        } while (buf[i++] != 0);
        n++;
    }
    r.beg = 0;
    r.end = n-1;
    PrintRange(r, 0);
}
