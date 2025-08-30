

http: src/http.c src/handleResponse.c
	gcc -I./include -Wall -Wextra -pedantic src/http.c src/handleResponse.c -lmagic -o http


