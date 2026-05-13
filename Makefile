obj-m := lsm6dsox_driver.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

REPORT = report_lsm6dsox_en

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

report: $(REPORT).tex
	pdflatex $(REPORT).tex
	pdflatex $(REPORT).tex

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.aux *.log *.out *.toc *.synctex.gz

clean_report:
	rm -f $(REPORT).aux $(REPORT).log $(REPORT).out $(REPORT).toc $(REPORT).synctex.gz $(REPORT).pdf
