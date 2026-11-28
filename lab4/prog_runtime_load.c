#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

typedef float (*calc_func_t)(int);

int main(void)
{
    const char *lib_files[2] = {"./libimpl1.dylib", "./libimpl2.dylib"};
    int current_idx = 0;

    void *handle = dlopen(lib_files[current_idx], RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "Ошибка загрузки %s: %s\n", lib_files[current_idx], dlerror());
        return EXIT_FAILURE;
    }

    calc_func_t fn_pi = (calc_func_t)dlsym(handle, "pi");
    calc_func_t fn_e = (calc_func_t)dlsym(handle, "e");
    if (!fn_pi || !fn_e)
    {
        fprintf(stderr, "Ошибка поиска символов: %s\n", dlerror());
        dlclose(handle);
        return EXIT_FAILURE;
    }

    int op = 0;
    int param = 0;

    while (scanf("%d", &op) == 1)
    {
        if (op == 0)
        {
            dlclose(handle);
            current_idx = (current_idx + 1) % 2;

            handle = dlopen(lib_files[current_idx], RTLD_LAZY);
            if (!handle)
            {
                fprintf(stderr, "Ошибка загрузки %s: %s\n", lib_files[current_idx], dlerror());
                return EXIT_FAILURE;
            }

            fn_pi = (calc_func_t)dlsym(handle, "pi");
            fn_e = (calc_func_t)dlsym(handle, "e");
            if (!fn_pi || !fn_e)
            {
                fprintf(stderr, "Ошибка поиска символов: %s\n", dlerror());
                dlclose(handle);
                return EXIT_FAILURE;
            }
            printf("Переключено на %s\n", lib_files[current_idx]);
        }
        else if (op == 1)
        {
            if (scanf("%d", &param) != 1)
                break;
            printf("pi(%d) = %.6f\n", param, fn_pi(param));
        }
        else if (op == 2)
        {
            if (scanf("%d", &param) != 1)
                break;
            printf("e(%d) = %.6f\n", param, fn_e(param));
        }
        else
        {
            printf("Неизвестная команда\n");
        }
    }

    dlclose(handle);
    return EXIT_SUCCESS;
}