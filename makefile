CC = gcc
CFLAGS =
OBJM = master.o
OBJP = producer.o fileHandler.o
OBJC = consumer.o fileHandler.o
MAIN = master
PROD = producer
CONS = consumer

.SUFFIXES: .c .o

all: $(MAIN) $(PROD) $(CONS)

$(MAIN): $(OBJM)
	$(CC) -o $@ $^

$(PROD): $(OBJP)
	$(CC) -o $@ $^

$(CONS): $(OBJC)
	$(CC) -o $@ $^

.c.o:
	$(CC) -c -o $@ $<

clean:
	rm *.o
	rm consumer
	rm master
	rm producer
	rm *.log
