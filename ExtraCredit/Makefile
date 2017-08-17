include Make.defines

PROGS =	 client server

OPTIONS = -DUNIX  -DANSI


COBJECTS =	DieWithError.o 
CSOURCES =	DieWithError.c 

CPLUSOBJECTS = 

COMMONSOURCES =

CPLUSSOURCES =

all:	${PROGS}


client:		PubSubClient.o $(CPLUSOBJECTS) $(COBJECTS) $(LIBS) $(COMMONSOURCES) $(SOURCES)
		${CC} ${LINKOPTIONS}  $@ PubSubClient.o $(CPLUSOBJECTS) $(COBJECTS) $(LIBS) $(LINKFLAGS)

server:		PubSubServer.o $(CPLUSOBJECTS) $(COBJECTS)
		${CC} ${LINKOPTIONS} $@ PubSubServer.o $(CPLUSOBJECTS) $(COBJECTS) $(LIBS) $(LINKFLAGS)


.cc.o:	$(HEADERS)
	$(CPLUS) $(CPLUSFLAGS) $(OPTIONS) $<

.c.o:	$(HEADERS)
	$(CC) $(CFLAGS) $(OPTIONS) $<



backup:
	rm -f PubSubREL.tar
	rm -f PubSubREL.tar.gz
	tar -cf PubSubREL.tar *
	gzip -f PubSubREL.tar

clean:
		rm -f ${PROGS} ${CLEANFILES}

depend:
		makedepend PubSubClient.c PubSubServer.c $(CFLAGS) $(HEADERS) $(SOURCES) $(COMMONSOURCES) $(CSOURCES)
#		mkdep $(CFLAGS) $(HEADERS) $(SOURCES) $(COMMONSOURCES) $(CSOURCES)

# DO NOT DELETE

