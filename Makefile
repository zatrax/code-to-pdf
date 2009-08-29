all: code2pdf pp
code2pdf: code2pdf.c
	gcc  -o $@ $< -lhpdf -lm -lz
pp: pp.c
	gcc -o $@ $<
