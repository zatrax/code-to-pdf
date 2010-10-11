# On Mac OS X Snow leopard, we need to specify i386 to link with libhpdf
CFLAGS=-Wunused -arch i386
all: code2pdf pp png2pdf
code2pdf: code2pdf.c
	gcc $(CFLAGS) -o $@ $< -lhpdf -lm -lz
png2pdf: png2pdf.c
	gcc -g $(CFLAGS) -o $@ $< -lhpdf -lm -lz -lpng
pp: pp.c
	gcc $(CFLAGS) -o $@ $<
clean:
	rm code2pdf pp
