

all: 
	gcc minishell.c -o minishell

run:
	./minishell

clean:
	rm -rf *.o *~ 