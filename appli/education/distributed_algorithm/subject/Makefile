NAME = TP-DistribAlg
PDF = $(patsubst %,%.pdf,$(NAME))
PDFFLAGS = -halt-on-error -file-line-error
all: $(PDF)

$(PDF): %.pdf: %.tex
	pdflatex $(PDFFLAGS) $*.tex

clean:
	$(RM) *.log *.aux *.out

realclean: clean
	$(RM) $(PDF)
