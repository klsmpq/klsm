PROJECT = paper

R      = R
LATEX  = pdflatex
BIBTEX = bibtex

RFILE = $(PROJECT).Rnw
AFILE = $(RFILE:.Rnw=.aux)
LFILE = $(RFILE:.Rnw=.tex)
PDF   = $(RFILE:.Rnw=.pdf)

all: $(PDF)

$(PDF): $(RFILE) acronyms.tex bibliography.bib pqplot.Rnw ../results/results.csv
	$(R) CMD Sweave $(RFILE)
	$(LATEX) $(LFILE)
	$(BIBTEX) $(AFILE)
	$(LATEX) $(LFILE)

clean:
	rm -f $(PROJECT)-blx.bib $(PROJECT).{aux,bbl,blg,log,out,tex} $(PROJECT).run.xml
