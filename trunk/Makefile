CFLAGS=-Wunused
all: code2pdf pp
code2pdf: code2pdf.c
	gcc $(CFLAGS) -o $@ $< -lhpdf -lm -lz
pp: pp.c
	gcc $(CFLAGS) -o $@ $<
clean:
	rm code2pdf pp