.PHONY: FORCE clean paper-manual paper-docker clean-manual

paper.pdf: FORCE
	latexrun paper.tex

warn:
	latexrun -Wall paper.tex

clean:
	latexrun --clean-all

paper-manual: paper.tex references.bib
	pdflatex paper.tex && \
	bibtex paper      && \
	pdflatex paper.tex && \
	pdflatex paper.tex

clean-amnual:
	rm -f *~ *.dvi paper.ps paper.pdf *.log *.aux *.toc *.nav *.out *.snm figures/*.aux figures/*.pdf

paper-docker: paper.tex references.bib
	docker-compose up
