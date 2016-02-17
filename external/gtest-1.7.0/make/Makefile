CC := g++
AR := ar


GTESTDIR = ..
BUILDDIR = ../../../build/external
LIBDIR := ../../../lib

INC = $(GTESTDIR)/include/gtest/*.h \
                $(GTESTDIR)/include/gtest/internal/*.h
GTEST_SRCS_ = $(GTESTDIR)/src/*.cc $(GTESTDIR)/src/*.h $(INC)
CFLAGS += -g -Wall -Werror -pthread -isystem $(GTESTDIR)/include 

all : $(LIBDIR)/libgtest_main.a $(LIBDIR)/libgtest.a

$(LIBDIR)/libgtest.a : $(BUILDDIR)/gtest-all.o
	@echo " $(AR) $(ARFLAGS) $@ $^"; $(AR) $(ARFLAGS) $@ $^
	
$(LIBDIR)/libgtest_main.a : $(BUILDDIR)/gtest-all.o $(BUILDDIR)/gtest_main.o
	@echo " $(AR) $(ARFLAGS) $@ $^"; $(AR) $(ARFLAGS) $@ $^

$(BUILDDIR)/gtest-all.o : $(GTEST_SRCS_)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) -I$(GTESTDIR) -c -o $@ $(GTESTDIR)/src/gtest-all.cc";\
		$(CC) $(CFLAGS) -I$(GTESTDIR) -c -o $@ $(GTESTDIR)/src/gtest-all.cc

$(BUILDDIR)/gtest_main.o : $(GTEST_SRCS_)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) -I$(GTESTDIR) -c -o $@ $(GTESTDIR)/src/gtest_main.cc";\
		$(CC) $(CFLAGS) -I$(GTESTDIR) -c -o $@ $(GTESTDIR)/src/gtest_main.cc
	
clean :
	@echo "Cleaning gtest..."
	@echo " rm -f $(LIBDIR)/libgtest*.a $(BUILDDIR)/*.o";\
		rm -f libgtest.a libgtest_main.a *.o
	
# Builds a sample test.  A test should link with either gtest.a or
# gtest_main.a, depending on whether it defines its own main()
# function.

#sample1.o : $(USER_DIR)/sample1.cc $(USER_DIR)/sample1.h $(INC)
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/sample1.cc

#sample1_unittest.o : $(USER_DIR)/sample1_unittest.cc \
#                     $(USER_DIR)/sample1.h $(INC)
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/sample1_unittest.cc

#sample1_unittest : sample1.o sample1_unittest.o gtest_main.a
#	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
