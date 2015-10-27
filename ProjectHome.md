# Introduction to code2pdf #

Code2pdf is a simple tool to print source code. It takes a source
file, and prints it as PDF file. It automatically sets the font size,
page margin and takes care of double-sided printing. The output could
be either A4 portrait or A4 landscape. The former has can have 64
lines per page, and each line hold up to 80 chars. Landscape one can
have 50 lines per page, each line up to 120 chars. Slim codes, often
seen in older programmers, can be printed in portrait. However,
nowadays, code become more and more fatter.


When printed in duplex,
  * 1000 lines take 8 pages in portrait, 10000 lines take 79 pages.
  * 1000 lines take 10 pages in landscape, 10000 lines take 100 pages.

doesn't wrap lines if a line is too long.

Odd pages and even pages have different page margins, so that enough
space is left for .  For odd pages, more space is left as left
margin. For even pages, more for right margin.

code2pdf also supports same margin for odd/even. This is most likely
you don't print in duplex. Think before you print. You are doubling
the paper usage. For printing more than ten pages, strongly urges you
to use duplex printing.

