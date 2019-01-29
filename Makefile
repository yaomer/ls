objs = main.o show.o get.o cmp.o error.o

myls : $(objs)
	cc -o myls $(objs)

.PHONY : clean
clean:
	-rm myls $(objs)
