#include "app/app_runner.h"
#include "daemon/daemon_runner.h"
#include "ota/ota_update.h"
#include <string.h>
#include <stdio.h>

void print_usage()
{
    printf("Usage: atguigu-gateway app|daemon|ota");
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        print_usage();
        return 1;
    }

    if (strcmp("app", argv[1]) == 0)
    {
        app_runner_run(argv[0]);
    }
    else if (strcmp("daemon", argv[1]) == 0)
    {
        daemon_runner_run(argv[0]);
    }
    else if (strcmp("ota", argv[1]) == 0)
    {
        ota_update_run();
    }
    else
    {
        print_usage();
        return 1;
    }

    return 0;
}
