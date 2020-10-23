objs = myls.o

myls : $(objs)
	cc -o myls $(objs)

.PHONY : clean
clean:
	-rm myls $(objs)
