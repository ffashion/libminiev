
head := include/event.h include/log.h include/list.h
obj := src/event.o src/log.o

libs := 
cflags := -g3 -fPIC -Wall -Wextra -Werror -Wno-implicit-fallthrough -Wno-stack-protector  -fsanitize=address -fno-omit-frame-pointer -fsanitize=leak 

all: miniev

%.o:%.c $(head)
	@ echo -e "  \e[32mCC\e[0m	" $@
	@ gcc -std=gnu11 -I ./include -o $@ -c $< $(libs) $(cflags)

miniev: $(obj)
	@ echo -e "  \e[34mMKELF\e[0m	" $@
	@ gcc -shared -o lib$@.so $^  $(libs) $(cflags)


clean:
	@ rm -f $(obj) libminiev.so
