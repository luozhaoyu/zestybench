default:
	gcc -g -Wall -o measure measure.c
	./measure

clean:
	rm -rf measure
