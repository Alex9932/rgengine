#if 0
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

class Base {
	public:
		Base() {
			//printf("Base constructor\n");
		}

		virtual ~Base() {
			//printf("Base destructor\n");
		}
};

class Child : public Base {
	public:
		Child() : Base() {
			//printf("Child constructor\n");
		}

		~Child() {
			//printf("Child destructor\n");
		}

};

#undef main
int main(int argc, char** argv) {

	SDL_Init(SDL_INIT_EVERYTHING);
	size_t len = 1024 * 2048;
	Base** pool = (Base**)malloc(sizeof(Base*) * len);

	Uint64 start = SDL_GetPerformanceCounter();

	for (size_t i = 0; i < len; i++) {
		pool[i] = new Child();
	}

	Uint64 end_new = SDL_GetPerformanceCounter();

	for (size_t i = 0; i < len; i++) {
		delete pool[i];
	}

	Uint64 end_delete = SDL_GetPerformanceCounter();

	double alloc_time = (double)(end_new - start) / (double)SDL_GetPerformanceFrequency();
	double free_time = (double)(end_delete - end_new) / (double)SDL_GetPerformanceFrequency();

	printf("Alloc: %lf\n", alloc_time);
	printf(" Free: %lf\n", free_time);


	free(pool);
	SDL_Quit();

	return 0;
}
#endif