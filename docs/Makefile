TARGETS := pdf
TARGETS += html

SRC  := block-api.rst

PDF  := $(SRC:%.rst=%.pdf)
HTML := $(SRC:%.rst=%.html)

all: $(TARGETS)

html: $(HTML)

pdf: $(PDF)

%.pdf: %.rst cp-style-sheet
	rst2pdf -s cp-style-sheet -o $@ $<

%.html: %.rst
	rst2html --time $< $@

clean:
	rm -rf $(PDF) $(HTM)
