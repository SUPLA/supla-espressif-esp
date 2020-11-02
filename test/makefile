csrc = $(wildcard gtest/*.c) $(wildcard src/*.c) $(wildcard mock/*.c)
ccsrc = $(wildcard gtest/*.cc) $(wildcard src/*.cc) $(wildcard mock/*.cc)
obj = $(csrc:.c=.o) $(ccsrc:.cc=.o)
dep = $(obj:.o=.d) 

CC = g++
CPPFLAGS += -I./ -I../src/include/board -I../src/user -I./mock -I./include -DGTEST_HAS_PTHREAD=0 -D__UTESTS__
CXXFLAGS += -g

LDFLAGS = -lpthread

utests: $(obj) 
	$(CXX) -o $@ $^ $(LDFLAGS)

-include $(dep)   # include all dep files in the makefile

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) utests

.PHONY: cleandep
cleandep:
	rm -f $(dep)
