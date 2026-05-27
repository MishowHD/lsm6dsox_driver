# 'obj-m' specifies that we want to build a kernel module. 
# The kernel's build system (Kbuild) will look for 'lsm6dsox_driver.o' 
# and produce 'lsm6dsox_driver.ko'.
obj-m := lsm6dsox_driver.o

# KDIR is the path to the kernel source or headers. 
# '$(shell uname -r)' gets the current kernel version.
# This ensures we compile against the same headers as the running kernel.
KDIR := /lib/modules/$(shell uname -r)/build

# PWD is the current directory where our source code lives.
PWD := $(shell pwd)

REPORT = report_lsm6dsox_en

# The 'all' target calls the kernel's Makefile (-C $(KDIR)).
# 'M=$(PWD)' tells the kernel Makefile to come back to this directory
# to find our source files and compile them as modules.
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Target to compile the LaTeX report.
# We run it twice to ensure cross-references (TOC, citations) are updated.
report: $(REPORT).tex
	pdflatex $(REPORT).tex
	pdflatex $(REPORT).tex

# 'clean' target delegates the cleanup to the kernel Makefile.
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.aux *.log *.out *.toc *.synctex.gz

# Cleanup only for LaTeX generated files.
clean_report:
	rm -f $(REPORT).aux $(REPORT).log $(REPORT).out $(REPORT).toc $(REPORT).synctex.gz $(REPORT).pdf
