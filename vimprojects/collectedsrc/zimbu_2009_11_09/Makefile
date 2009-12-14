# Specify the C compiler to be used
COMPILER = $(CC)

SOURCES = \
	  attr.zu \
	  builtin.zu \
	  config.zu \
	  conversion.zu \
	  dictstuff.zu \
	  error.zu \
	  expreval.zu \
	  generate.zu \
	  input.zu \
	  lib/httploader.zu \
	  lib/httpmodule.zu \
	  lib/infoloader.zu \
	  lib/infomodule.zu \
	  lib/libarg.zu \
	  lib/libio.zu \
	  lib/libsys.zu \
	  lib/libthread.zu \
	  lib/zwtloader.zu \
	  lib/zwtmodule.zu \
	  liststuff.zu \
	  node.zu \
	  output.zu \
	  parse.zu \
	  pos.zu \
	  resolve.zu \
	  scontext.zu \
	  scope.zu \
	  symbol.zu \
	  token.zu \
	  tokenize.zu \
	  usedfile.zu \
	  write_c.zu \
	  write_js.zu \
	  zimbufile.zu \
	  zimbu.zu


# Default target: test building the new version using the tested version
zimbu_test: $(SOURCES)
	cp -f zimbu.zu zimbu_test.zu
	./zimbu -cc $(COMPILER) zimbu_test.zu

# Pack up the sources and generated C files to move elsewhere.
# NOTE: depends on zimbu_test and uses zimbu_test_zimbu.c, because depending on
# zimbu may break the compiler and zimbu.c may be out of sync with the other C
# files.
# Best is to do "make zimbu" though (after testing!).
package: zimbu_test
	cp -f zimbu_test_zimbu.c zimbu.c
	cp -f ~/.vim/syntax/zimbu.vim vim/syntax
	cp -f ~/.vim/indent/zimbu.vim vim/indent
	cp -f ~/.vim/ftplugin/zimbu.vim vim/ftplugin
	dir=`date +zimbu_%Y_%m_%d`; mkdir $$dir; \
	    tar cf - \
		*.zu lib/*.zu zimbush \
		testdir/*.zu testdir/subdir/*.zu test.ok \
		zimbu.c ZUDIR/*.c ZUDIR/*.h lib/ZUDIR/*.c lib/ZUDIR/*.h \
		*.txt Makefile README lib/README LICENSE \
		vim/*/zimbu.vim vim/filetype.vim \
		| tar xf - -C $$dir; \
	   tar cvfz $$dir.tgz $$dir; \
	   rm -r $$dir

# When on a new system: Build Zimbu from the C files.
# TODO: this should build and run a configure program first.
bootstrap:
	$(COMPILER) -o zimbu zimbu.c

# Test some things
test: test_testprog test_testprog_zimbu

# Test that the new version can build and run the test program
test_testprog: zimbu_test
	@echo
	./zimbu_test -v -cc $(COMPILER) testprog.zu
	@echo
	./testprog argone argtwo > test.out
	cat test.out
	diff test.ok test.out

# Test that the new version can build itself
zimbu_test_zimbu: zimbu_test
	@echo
	cp -f zimbu.zu zimbu_test_zimbu.zu
	./zimbu_test -v -cc $(COMPILER) zimbu_test_zimbu.zu

# Test that the version that build itself can build and run the test program
test_testprog_zimbu: zimbu_test_zimbu
	@echo
	./zimbu_test_zimbu -v -cc $(COMPILER) testprog.zu
	@echo
	./testprog argone argtwo > test.out
	cat test.out
	diff test.ok test.out

# Only do this when "make test" succeeded!
zimbu: $(SOURCES)
	./zimbu -cc $(COMPILER) zimbu.zu

testprog: testprog.zu zimbu testdir/*.zu
	@echo
	./zimbu -cc $(COMPILER) testprog.zu

# A few example programs

examples: echo copy animal hello testserver

echo: echo.zu zimbu_test
	./zimbu_test -cc $(COMPILER) echo.zu

copy: copy.zu zimbu_test
	./zimbu_test -cc $(COMPILER) copy.zu

animal: animal.zu zimbu_test
	./zimbu_test -cc $(COMPILER) animal.zu

hello: hello.zu zimbu_test
	./zimbu_test -cc $(COMPILER) hello.zu

testserver: testserver.zu zimbu_test
	./zimbu_test -cc $(COMPILER) testserver.zu

zwt_demo: zwt_demo.zu zwtDemoPage.zu zimbu_test
	./zimbu_test -v -cc $(COMPILER) zwt_demo.zu

testje: testje.zu zimbu_test
	./zimbu_test -v -cc $(COMPILER) testje.zu

tags: *.c *.zu
	ctags \
		--langdef=zimbu \
		--langmap=zimbu:.zu \
		'--regex-zimbu=/^ *FUNC +[^ ]+ +([^(]+)/\1/' \
		'--regex-zimbu=/^ *PROC +([^(]+)/\1/' \
		'--regex-zimbu=/^ *ENUM +([^ ]+)/\1/' \
		'--regex-zimbu=/^ *CLASS +([^ ]+)/\1/' \
		'--regex-zimbu=/^ *MODULE +([^ ]+)/\1/' \
		'--regex-zimbu=/^ *[a-zA-Z][a-z_]+ +([a-zA-Z][^ ]+)/\1/' \
		*.zu

backup: doesnotexist
	-mkdir backup/`date "+%Y-%m-%d-%H:%M"`
	cp -r zimbu *.zu *.txt Makefile lib testdir test.ok backup/`date "+%Y-%m-%d-%H:%M"`

doesnotexist:
