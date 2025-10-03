all: checkmakefiles
	cd src && $(MAKE)

clean: checkmakefiles
	cd src && $(MAKE) clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	rm -f src/Makefile

INET_PROJ=../../inet4.5
SIMU5G_PROJ=../../simu5g
GUROBI_INC=$(GUROBI_HOME)/include
GUROBI_LIB=$(GUROBI_HOME)/lib
GUROBI_VER=120
makefiles:
	@cd src && opp_makemake --make-so -f --deep -o mecrt -O out \
	-KINET_PROJ=$(INET_PROJ) -KSIMU5G_PROJ=$(SIMU5G_PROJ) \
	-DINET_IMPORT -DSIMU5G_IMPORT \
	-I. \
	-I$(INET_PROJ)/src -L$(INET_PROJ)/src -lINET \
	-I$(SIMU5G_PROJ)/src -L$(SIMU5G_PROJ)/src -lsimu5g \
	-I$(GUROBI_INC) -L$(GUROBI_LIB) -lgurobi_c++ -lgurobi$(GUROBI_VER)
# 	@cd src && opp_makemake --make-so -f --deep -o mecrt -O out \
# 	-KINET_PROJ=$(INET_PROJ) -KSIMU5G_PROJ=$(SIMU5G_PROJ) \
# 	-DINET_IMPORT -DSIMU5G_IMPORT \
# 	-I. \
# 	-I$(INET_PROJ)/src -L$(INET_PROJ)/src -lINET_dbg \
# 	-I$(SIMU5G_PROJ)/src -L$(SIMU5G_PROJ)/src -lsimu5g_dbg \
# 	-I$(GUROBI_INC) -L$(GUROBI_LIB) -lgurobi_c++ -lgurobi$(GUROBI_VER)
	

checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi
